/**
 * @file test_communication_protocol.cpp
 * @brief Unit test per model/CommunicationProtocol: costruzione comandi e
 *        parsing delle risposte (incluso il checksum XOR stile NMEA).
 */
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

#include "TestFramework.h"
#include "Version.h"
#include "model/CommunicationProtocol.h"

using am::CommunicationProtocol;
using am::ResponseType;

// --- Costruzione comandi ---------------------------------------------------

TEST_CASE("CommunicationProtocol: build* producono le righe di comando attese")
{
    CHECK_EQ(CommunicationProtocol::buildHello(), std::string("HELLO"));
    CHECK_EQ(CommunicationProtocol::buildVersionRequest(), std::string("VER"));
    CHECK_EQ(CommunicationProtocol::buildGet(), std::string("GET"));
    CHECK_EQ(CommunicationProtocol::buildSetOutput(5, true), std::string("SET D5 1"));
    CHECK_EQ(CommunicationProtocol::buildSetOutput(9, false), std::string("SET D9 0"));
    CHECK_EQ(CommunicationProtocol::buildRate(250), std::string("RATE 250"));
    CHECK_EQ(CommunicationProtocol::buildStream(true), std::string("STREAM 1"));
    CHECK_EQ(CommunicationProtocol::buildStream(false), std::string("STREAM 0"));
}

// --- Checksum ----------------------------------------------------------------

TEST_CASE("CommunicationProtocol: computeXor")
{
    CHECK_EQ(CommunicationProtocol::computeXor(""), std::uint8_t{ 0 });
    CHECK_EQ(CommunicationProtocol::computeXor("A"), std::uint8_t{ 'A' });
    // XOR di se stesso con se stesso (stringhe uguali carattere per carattere
    // in posizioni diverse) non e' 0 in generale: verifica solo un caso noto.
    CHECK_EQ(CommunicationProtocol::computeXor("AA"), std::uint8_t{ 0 });
}

// --- Righe vuote / malformate -------------------------------------------------

TEST_CASE("CommunicationProtocol: righe vuote o solo spazi -> nullopt (non un errore)")
{
    CHECK(!CommunicationProtocol::parseLine("").has_value());
    CHECK(!CommunicationProtocol::parseLine("   ").has_value());
    CHECK(!CommunicationProtocol::parseLine("\r").has_value());
}

TEST_CASE("CommunicationProtocol: riga non riconosciuta -> Unknown")
{
    const auto r = CommunicationProtocol::parseLine("PIPPO PLUTO");
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::Unknown);
}

// --- Handshake / Version / Error ----------------------------------------------

TEST_CASE("CommunicationProtocol: ARDUINO_UNO -> Handshake")
{
    const auto r = CommunicationProtocol::parseLine("ARDUINO_UNO");
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::Handshake);
}

TEST_CASE("CommunicationProtocol: VERSION x.y.z -> Version")
{
    const auto r = CommunicationProtocol::parseLine("VERSION 1.0.0\r");
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::Version);
    CHECK_EQ(r->version, std::string("1.0.0"));
}

TEST_CASE("CommunicationProtocol: ERR <msg> -> Error col messaggio")
{
    const auto r = CommunicationProtocol::parseLine("ERR RATE RANGE");
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::Error);
    CHECK_EQ(r->message, std::string("RATE RANGE"));
}

TEST_CASE("CommunicationProtocol: ERR senza messaggio non esplode")
{
    const auto r = CommunicationProtocol::parseLine("ERR");
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::Error);
    CHECK_EQ(r->message, std::string(""));
}

// --- OK Dx / OK RATE / OK STREAM ----------------------------------------------

TEST_CASE("CommunicationProtocol: OK D<pin>=<v> -> OkOutput")
{
    const auto r1 = CommunicationProtocol::parseLine("OK D5=1");
    CHECK(r1.has_value());
    CHECK(r1->type == ResponseType::OkOutput);
    CHECK_EQ(r1->pin, 5);
    CHECK(r1->pinState == true);

    const auto r0 = CommunicationProtocol::parseLine("OK D9=0");
    CHECK(r0.has_value());
    CHECK(r0->type == ResponseType::OkOutput);
    CHECK_EQ(r0->pin, 9);
    CHECK(r0->pinState == false);
}

TEST_CASE("CommunicationProtocol: OK RATE=<hz> -> OkRate")
{
    const auto r = CommunicationProtocol::parseLine("OK RATE=250");
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::OkRate);
    CHECK_EQ(r->rateHz, 250);
}

TEST_CASE("CommunicationProtocol: OK STREAM=<v> -> OkStream")
{
    const auto r1 = CommunicationProtocol::parseLine("OK STREAM=1");
    CHECK(r1.has_value());
    CHECK(r1->type == ResponseType::OkStream);
    CHECK(r1->streaming == true);

    const auto r0 = CommunicationProtocol::parseLine("OK STREAM=0");
    CHECK(r0.has_value());
    CHECK(r0->type == ResponseType::OkStream);
    CHECK(r0->streaming == false);
}

TEST_CASE("CommunicationProtocol: OK STREAM=2 (valore invalido) -> Unknown")
{
    const auto r = CommunicationProtocol::parseLine("OK STREAM=2");
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::Unknown);
}

// --- ADC: costruzione di una riga valida con lo stesso XOR del firmware -------

namespace {

/// Costruisce una riga "ADC,v0,...,v5*CS" con checksum corretto, replicando
/// esattamente la logica di sendAdcFrame() in AliveMonitor.ino: utile per
/// verificare che il parser PC-side sia compatibile col firmware reale senza
/// dover linkare/eseguire lo sketch.
std::string buildAdcLine(const std::array<int, am::kNumAnalogChannels>& values)
{
    std::string payload = "ADC";
    for (const int v : values) {
        payload += ',' + std::to_string(v);
    }
    const std::uint8_t cs = CommunicationProtocol::computeXor(payload);
    char hex[3];
    std::snprintf(hex, sizeof(hex), "%02X", cs);
    return payload + '*' + hex;
}

} // namespace

TEST_CASE("CommunicationProtocol: riga ADC con checksum valido -> Adc")
{
    const std::array<int, am::kNumAnalogChannels> values{ 0, 100, 511, 1023, 42, 999 };
    const std::string line = buildAdcLine(values);
    const auto r = CommunicationProtocol::parseLine(line);
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::Adc);
    for (int i = 0; i < am::kNumAnalogChannels; ++i) {
        CHECK_EQ(r->adcValues[static_cast<std::size_t>(i)],
                static_cast<std::uint16_t>(values[static_cast<std::size_t>(i)]));
    }
}

TEST_CASE("CommunicationProtocol: riga ADC con checksum errato -> ChecksumError")
{
    const std::array<int, am::kNumAnalogChannels> values{ 1, 2, 3, 4, 5, 6 };
    std::string line = buildAdcLine(values);
    // Corrompe l'ultima cifra del checksum.
    line.back() = (line.back() == '0') ? '1' : '0';
    const auto r = CommunicationProtocol::parseLine(line);
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::ChecksumError);
}

TEST_CASE("CommunicationProtocol: riga ADC con un valore fuori range -> Unknown")
{
    // kAdcMaxValue e' 1023: 1024 e' fuori range e deve fallire il parsing
    // (senza checksum: verifica solo la validazione dei valori).
    const auto r = CommunicationProtocol::parseLine("ADC,0,0,0,0,0,1024");
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::Unknown);
}

TEST_CASE("CommunicationProtocol: riga ADC con troppi pochi valori -> Unknown")
{
    const auto r = CommunicationProtocol::parseLine("ADC,1,2,3,4,5");  // solo 5, ne servono 6
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::Unknown);
}

TEST_CASE("CommunicationProtocol: riga ADC con troppi valori -> Unknown")
{
    const auto r = CommunicationProtocol::parseLine("ADC,1,2,3,4,5,6,7");  // 7, ne servono 6
    CHECK(r.has_value());
    CHECK(r->type == ResponseType::Unknown);
}
