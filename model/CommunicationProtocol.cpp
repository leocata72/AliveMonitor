/**
 * @file CommunicationProtocol.cpp
 * @brief Implementazione del protocollo testuale PC <-> Arduino.
 */
#include "model/CommunicationProtocol.h"

#include <charconv>

namespace am {
namespace {

/// Rimuove spazi e '\r' ai bordi della riga.
[[nodiscard]] std::string_view trim(std::string_view s) noexcept
{
    constexpr std::string_view ws = " \t\r\n";
    const auto first = s.find_first_not_of(ws);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = s.find_last_not_of(ws);
    return s.substr(first, last - first + 1);
}

/// Parsing di un intero senza eccezioni. nullopt se non valido.
[[nodiscard]] std::optional<int> toInt(std::string_view s) noexcept
{
    int value = 0;
    const auto* begin = s.data();
    const auto* end = s.data() + s.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }
    return value;
}

/// Converte due cifre esadecimali in un byte. nullopt se non valide.
[[nodiscard]] std::optional<std::uint8_t> hexByte(std::string_view s) noexcept
{
    if (s.size() != 2) {
        return std::nullopt;
    }
    unsigned value = 0;
    const auto [ptr, ec] = std::from_chars(s.data(), s.data() + 2, value, 16);
    if (ec != std::errc{} || ptr != s.data() + 2) {
        return std::nullopt;
    }
    return static_cast<std::uint8_t>(value);
}

} // namespace

// ---------------------------------------------------------------------------
// Costruzione comandi
// ---------------------------------------------------------------------------

std::string CommunicationProtocol::buildHello()
{
    return std::string(kHello);
}

std::string CommunicationProtocol::buildVersionRequest()
{
    return "VER";
}

std::string CommunicationProtocol::buildGet()
{
    return "GET";
}

std::string CommunicationProtocol::buildSetOutput(int pin, bool on)
{
    return "SET D" + std::to_string(pin) + (on ? " 1" : " 0");
}

std::string CommunicationProtocol::buildSetDirection(int pin, bool input)
{
    return "DIR D" + std::to_string(pin) + (input ? " I" : " O");
}

std::string CommunicationProtocol::buildRate(int rateHz)
{
    return "RATE " + std::to_string(rateHz);
}

std::string CommunicationProtocol::buildStream(bool on)
{
    return on ? "STREAM 1" : "STREAM 0";
}

// ---------------------------------------------------------------------------
// Checksum
// ---------------------------------------------------------------------------

std::uint8_t CommunicationProtocol::computeXor(std::string_view payload) noexcept
{
    std::uint8_t cs = 0;
    for (const char c : payload) {
        cs = static_cast<std::uint8_t>(cs ^ static_cast<std::uint8_t>(c));
    }
    return cs;
}

// ---------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------

std::optional<ParsedResponse> CommunicationProtocol::parseLine(std::string_view rawLine)
{
    const std::string_view line = trim(rawLine);
    if (line.empty()) {
        return std::nullopt;  // righe vuote: nessun errore, semplicemente ignorate
    }

    ParsedResponse r;

    if (line == kHandshakeReply) {
        r.type = ResponseType::Handshake;
        return r;
    }
    if (line.rfind("ADC,", 0) == 0) {
        return parseAdc(line);
    }
    if (line.rfind("OK ", 0) == 0) {
        return parseOk(line.substr(3));
    }
    if (line.rfind("IN D", 0) == 0) {
        // Notifica spontanea del livello di un pin in ingresso: "IN D<pin>=<0|1>".
        const auto eq = line.find('=');
        if (eq != std::string_view::npos && eq > 4) {
            const auto pin = toInt(trim(line.substr(4, eq - 4)));
            const auto v = toInt(trim(line.substr(eq + 1)));
            if (pin && v && (*v == 0 || *v == 1)) {
                r.type = ResponseType::InputState;
                r.pin = *pin;
                r.pinState = (*v == 1);
                return r;
            }
        }
        r.type = ResponseType::Unknown;
        return r;
    }
    if (line.rfind("VERSION ", 0) == 0) {
        r.type = ResponseType::Version;
        r.version = std::string(trim(line.substr(8)));
        return r;
    }
    if (line.rfind("ERR", 0) == 0) {
        r.type = ResponseType::Error;
        r.message = std::string(trim(line.size() > 3 ? line.substr(3) : ""));
        return r;
    }

    r.type = ResponseType::Unknown;
    return r;
}

ParsedResponse CommunicationProtocol::parseAdc(std::string_view line)
{
    ParsedResponse r;
    r.type = ResponseType::Unknown;

    // Separazione payload / checksum ("ADC,...*HH"). Il checksum è opzionale
    // per retro-compatibilità, ma il firmware fornito lo invia sempre.
    std::string_view payload = line;
    const auto star = line.rfind('*');
    if (star != std::string_view::npos) {
        payload = line.substr(0, star);
        const auto expected = hexByte(trim(line.substr(star + 1)));
        if (!expected || computeXor(payload) != *expected) {
            r.type = ResponseType::ChecksumError;
            return r;
        }
    }

    // Parsing dei sei valori dopo "ADC,".
    std::string_view rest = payload.substr(4);
    std::array<std::uint16_t, kNumAnalogChannels> values{};
    for (int i = 0; i < kNumAnalogChannels; ++i) {
        const auto comma = rest.find(',');
        const std::string_view token =
            (comma == std::string_view::npos) ? rest : rest.substr(0, comma);

        const auto v = toInt(token);
        if (!v || *v < 0 || *v > kAdcMaxValue) {
            return r;  // malformata -> Unknown
        }
        values[static_cast<std::size_t>(i)] = static_cast<std::uint16_t>(*v);

        if (comma == std::string_view::npos) {
            // Ultimo token: deve essere davvero il sesto.
            if (i != kNumAnalogChannels - 1) {
                return r;
            }
            rest = {};
        } else {
            // C'è una virgola ma era l'ultimo atteso -> troppi valori.
            if (i == kNumAnalogChannels - 1) {
                return r;
            }
            rest = rest.substr(comma + 1);
        }
    }

    r.type = ResponseType::Adc;
    r.adcValues = values;
    return r;
}

ParsedResponse CommunicationProtocol::parseOk(std::string_view body)
{
    ParsedResponse r;
    r.type = ResponseType::Unknown;

    const auto eq = body.find('=');
    if (eq == std::string_view::npos) {
        return r;
    }
    const std::string_view key = trim(body.substr(0, eq));
    const std::string_view value = trim(body.substr(eq + 1));

    // "OK DIR D<pin>=<I|O>": conferma di direzione (v1.2). Va controllata
    // PRIMA del ramo "D<pin>" (la chiave inizia per 'D' solo dopo "DIR ").
    if (key.rfind("DIR D", 0) == 0) {
        const auto pin = toInt(key.substr(5));
        if (pin && (value == "I" || value == "O")) {
            r.type = ResponseType::OkDirection;
            r.pin = *pin;
            r.isInput = (value == "I");
        }
        return r;
    }

    if (key.size() >= 2 && key[0] == 'D') {
        const auto pin = toInt(key.substr(1));
        const auto v = toInt(value);
        if (pin && v && (*v == 0 || *v == 1)) {
            r.type = ResponseType::OkOutput;
            r.pin = *pin;
            r.pinState = (*v == 1);
        }
        return r;
    }
    if (key == "RATE") {
        if (const auto v = toInt(value)) {
            r.type = ResponseType::OkRate;
            r.rateHz = *v;
        }
        return r;
    }
    if (key == "STREAM") {
        const auto v = toInt(value);
        if (v && (*v == 0 || *v == 1)) {
            r.type = ResponseType::OkStream;
            r.streaming = (*v == 1);
        }
        return r;
    }
    return r;
}

} // namespace am
