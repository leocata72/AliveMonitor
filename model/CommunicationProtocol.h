/**
 * @file CommunicationProtocol.h
 * @brief Protocollo testuale PC <-> Arduino: costruzione comandi e parsing risposte.
 *
 * MODEL (MVC). Classe SENZA stato (soli metodi statici): può essere usata da
 * qualunque thread senza sincronizzazione. È l'unico punto del programma che
 * conosce la sintassi del protocollo: aggiungere un comando (PWM, SERVO, I2C,
 * ...) significa aggiungere qui un builder e/o un ramo di parsing, senza
 * toccare il resto dell'applicazione.
 *
 * Sintassi (righe ASCII terminate da '\n'):
 *   PC -> Arduino:  HELLO | VER | GET | SET D<pin> <0|1> | DIR D<pin> <I|O> |
 *                   RATE <hz> | STREAM <0|1>
 *   Arduino -> PC:  ARDUINO_UNO | VERSION <x.y.z> | ADC,v0,...,v5*CS |
 *                   OK D<pin>=<0|1> | OK DIR D<pin>=<I|O> | IN D<pin>=<0|1> |
 *                   OK RATE=<hz> | OK STREAM=<0|1> | ERR <msg>
 *
 * "IN D<pin>=<0|1>" è una notifica SPONTANEA del firmware: livello corrente
 * di un pin configurato come ingresso, inviata a ogni cambio (debounce) e
 * subito dopo l'OK di un "DIR ... I".
 *
 * Le righe ADC portano un checksum stile NMEA: '*' seguito da due cifre
 * esadecimali = XOR di tutti i caratteri precedenti l'asterisco.
 */
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "Version.h"

namespace am {

/// Esito del parsing di una riga ricevuta.
enum class ResponseType {
    Handshake,      ///< "ARDUINO_UNO"
    Adc,            ///< "ADC,..." con checksum valido
    OkOutput,       ///< "OK Dx=v"
    OkDirection,    ///< "OK DIR Dx=I/O" (v1.2)
    InputState,     ///< "IN Dx=v": livello di un pin in ingresso (v1.2)
    OkRate,         ///< "OK RATE=n"
    OkStream,       ///< "OK STREAM=v"
    Version,        ///< "VERSION x.y.z"
    Error,          ///< "ERR ..."
    ChecksumError,  ///< Riga ADC con checksum errato
    Unknown         ///< Riga non riconosciuta / malformata
};

/// Risultato del parsing. I campi sono validi in funzione di `type`.
struct ParsedResponse {
    ResponseType type = ResponseType::Unknown;
    std::array<std::uint16_t, kNumAnalogChannels> adcValues{};  ///< se Adc
    int pin = 0;              ///< se OkOutput / OkDirection / InputState
    bool pinState = false;    ///< se OkOutput / InputState
    bool isInput = false;     ///< se OkDirection (true = ingresso)
    int rateHz = 0;           ///< se OkRate
    bool streaming = false;   ///< se OkStream
    std::string version;      ///< se Version
    std::string message;      ///< se Error (testo dopo "ERR")
};

class CommunicationProtocol {
public:
    CommunicationProtocol() = delete;  ///< Solo metodi statici.

    // --- Costanti del protocollo ---------------------------------------------
    static constexpr std::string_view kHello = "HELLO";
    static constexpr std::string_view kHandshakeReply = "ARDUINO_UNO";

    // --- Costruzione comandi (senza terminatore '\n') --------------------------
    [[nodiscard]] static std::string buildHello();
    [[nodiscard]] static std::string buildVersionRequest();
    [[nodiscard]] static std::string buildGet();
    [[nodiscard]] static std::string buildSetOutput(int pin, bool on);
    /// "DIR D<pin> I" (ingresso) oppure "DIR D<pin> O" (uscita). (v1.2)
    [[nodiscard]] static std::string buildSetDirection(int pin, bool input);
    [[nodiscard]] static std::string buildRate(int rateHz);
    [[nodiscard]] static std::string buildStream(bool on);

    // --- Parsing --------------------------------------------------------------
    /// Analizza una riga già privata del terminatore. Restituisce nullopt
    /// per righe vuote (da ignorare senza contare errori).
    [[nodiscard]] static std::optional<ParsedResponse> parseLine(std::string_view line);

    /// XOR di tutti i caratteri di `payload` (checksum stile NMEA).
    [[nodiscard]] static std::uint8_t computeXor(std::string_view payload) noexcept;

private:
    [[nodiscard]] static ParsedResponse parseAdc(std::string_view line);
    [[nodiscard]] static ParsedResponse parseOk(std::string_view line);
};

} // namespace am
