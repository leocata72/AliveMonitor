/**
 * @file Version.h
 * @brief Costanti globali di configurazione dell'applicazione.
 *
 * Unico punto di verità per nome, versione e parametri hardware/protocollo.
 * Nessuna variabile globale mutabile: solo costanti `constexpr` in namespace.
 */
#pragma once

#include <cstddef>

namespace am {

/// Nome dell'applicazione (titolo finestra, dialoghi).
inline constexpr const char* kAppName = "AliveMonitor";

/// Versione dell'applicazione PC.
inline constexpr const char* kAppVersion = "1.3.0";

/// Baud rate predefinito della comunicazione con Arduino Uno.
inline constexpr unsigned kDefaultBaudRate = 115200U;

/// Numero di ingressi analogici acquisiti (A0..A5).
inline constexpr int kNumAnalogChannels = 6;

/// Numero di uscite digitali comandabili (D2..D9).
inline constexpr int kNumDigitalOutputs = 8;

/// Primo pin digitale comandabile.
inline constexpr int kFirstDigitalPin = 2;

/// Tensione di riferimento dell'ADC di Arduino Uno [V].
inline constexpr double kAdcReferenceVolt = 5.0;

/// Valore massimo dell'ADC a 10 bit.
inline constexpr int kAdcMaxValue = 1023;

/// Frequenza minima di acquisizione [Hz].
inline constexpr int kMinSampleRateHz = 1;

/// Frequenza massima di acquisizione attuale [Hz].
inline constexpr int kMaxSampleRateHz = 250;

/// Frequenza massima futura per cui i buffer sono già dimensionati [Hz].
inline constexpr int kFutureMaxSampleRateHz = 500;

/// Finestra temporale minima garantita dai ring buffer [s].
inline constexpr int kBufferSeconds = 60;

/// Capacità di ogni ring buffer: 60 s a 500 Hz (predisposizione futura).
inline constexpr std::size_t kRingBufferCapacity =
    static_cast<std::size_t>(kBufferSeconds) * kFutureMaxSampleRateHz;

/// FPS minimo/massimo/di default del rendering del grafico.
inline constexpr int kMinRenderFps = 10;
inline constexpr int kMaxRenderFps = 60;
inline constexpr int kDefaultRenderFps = 30;

/// Frequenza di acquisizione predefinita [Hz].
inline constexpr int kDefaultSampleRateHz = 100;

} // namespace am
