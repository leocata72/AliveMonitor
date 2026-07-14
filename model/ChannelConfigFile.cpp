/**
 * @file ChannelConfigFile.cpp
 * @brief Implementazione della persistenza testuale della configurazione canali.
 */
#include "model/ChannelConfigFile.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <string_view>

namespace am {
namespace {

/// Rimuove spazi/tab/CR ai bordi (il file può arrivare da Windows o Unix).
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

/// Numero -> testo con il punto decimale, indipendente dalla locale
/// (snprintf con "%.10g" usa sempre il punto con la locale "C" di default
/// del processo; wxWidgets non cambia la locale numerica del C runtime).
[[nodiscard]] std::string formatDouble(double v)
{
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%.10g", v);
    return std::string(buf);
}

/// Testo -> double accettando SOLO il punto come separatore decimale.
/// std::strtod è locale-dipendente, quindi qui il parsing è manuale-ma-solido:
/// si convalida il formato ([-+]?digits[.digits][e[-+]digits]) e si converte
/// con strtod dopo aver verificato che non ci siano virgole; con locale "C"
/// (default di questo processo) il risultato è comunque corretto.
[[nodiscard]] bool parseDouble(std::string_view s, double& out)
{
    if (s.empty() || s.find(',') != std::string_view::npos) {
        return false;
    }
    const std::string tmp(s);
    char* end = nullptr;
    const double v = std::strtod(tmp.c_str(), &end);
    if (end != tmp.c_str() + tmp.size()) {
        return false;
    }
    out = v;
    return true;
}

} // namespace

bool ChannelConfigFile::save(const std::string& path,
                             const ChannelCalibrations& calibrations)
{
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file << "# AliveMonitor - configurazione canali analogici (v1)\n";
    file << "# a, b: G = a*V + b   unit: unita' di misura   label: nome canale\n";
    file << "# marker: 0=nessuno 1=cerchio pieno 2=cerchio vuoto 3=quadrato\n";
    file << "#         4=triangolo 5=rombo 6=stella 7=croce 8=croce diagonale\n";
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        const auto& c = calibrations[static_cast<std::size_t>(ch)];
        file << "\n[A" << ch << "]\n";
        file << "a=" << formatDouble(c.a) << '\n';
        file << "b=" << formatDouble(c.b) << '\n';
        file << "unit=" << c.unit << '\n';
        file << "label=" << c.label << '\n';
        file << "marker=" << static_cast<int>(c.marker) << '\n';
    }
    return static_cast<bool>(file);
}

bool ChannelConfigFile::load(const std::string& path,
                             ChannelCalibrations& calibrations)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    // Le modifiche vengono accumulate su una COPIA e applicate solo alla
    // fine: un file che si rivela non valido a metà (nessuna sezione
    // riconosciuta) non lascia la configurazione corrente mezza sovrascritta.
    ChannelCalibrations updated = calibrations;

    int currentChannel = -1;   // -1 = fuori da ogni sezione [Ax] valida
    bool anySection = false;

    std::string rawLine;
    while (std::getline(file, rawLine)) {
        const std::string_view line = trim(rawLine);
        if (line.empty() || line.front() == '#' || line.front() == ';') {
            continue;  // vuota o commento
        }

        // Intestazione di sezione: "[A0]".."[A5]". Sezioni sconosciute
        // (estensioni future) vengono saltate senza errore.
        if (line.front() == '[' && line.back() == ']') {
            const std::string_view name = trim(line.substr(1, line.size() - 2));
            currentChannel = -1;
            if (name.size() >= 2 && (name[0] == 'A' || name[0] == 'a')) {
                const char digit = name[1];
                if (name.size() == 2 && digit >= '0'
                    && digit < '0' + kNumAnalogChannels) {
                    currentChannel = digit - '0';
                    anySection = true;
                }
            }
            continue;
        }

        if (currentChannel < 0) {
            continue;  // riga chiave=valore fuori da una sezione canale nota
        }

        const auto eq = line.find('=');
        if (eq == std::string_view::npos) {
            continue;  // riga non riconosciuta: tollerata
        }
        const std::string_view key = trim(line.substr(0, eq));
        const std::string_view value = trim(line.substr(eq + 1));
        auto& c = updated[static_cast<std::size_t>(currentChannel)];

        if (key == "a") {
            double v = 0.0;
            if (parseDouble(value, v)) {
                c.a = v;
            }
        } else if (key == "b") {
            double v = 0.0;
            if (parseDouble(value, v)) {
                c.b = v;
            }
        } else if (key == "unit") {
            // Unità vuota -> "V" (coerente con la griglia, che non permette
            // di lasciare l'unità vuota: vedi CalibrationPanel).
            c.unit = value.empty() ? std::string("V") : std::string(value);
        } else if (key == "label") {
            c.label = std::string(value);
        } else if (key == "marker") {
            // Indice numerico dell'enum MarkerStyle; valori fuori range
            // (file scritto a mano o da una versione futura con più stili)
            // vengono ignorati lasciando il marker corrente.
            double v = 0.0;
            if (parseDouble(value, v)) {
                const int idx = static_cast<int>(v);
                if (idx >= 0 && idx < kMarkerStyleCount
                    && static_cast<double>(idx) == v) {
                    c.marker = static_cast<MarkerStyle>(idx);
                }
            }
        }
        // Chiavi sconosciute (estensioni future): ignorate senza errore.
    }

    if (!anySection) {
        return false;  // nessuna [A0]..[A5]: non è un file di configurazione
    }
    calibrations = updated;
    return true;
}

} // namespace am
