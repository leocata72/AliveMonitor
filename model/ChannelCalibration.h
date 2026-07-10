/**
 * @file ChannelCalibration.h
 * @brief Calibrazione lineare di un canale analogico (Volt -> grandezza fisica).
 *
 * MODEL (MVC). Dato puro, nessuna logica: converte la tensione letta
 * dall'ADC in una grandezza fisica G = a*V + b, con la relativa unità di
 * misura. Utilizzato per etichettare la legenda del grafico e per le
 * colonne aggiuntive nel CSV.
 *
 * Accesso SOLO dal thread GUI: la griglia di configurazione
 * (CalibrationPanel), la legenda del grafico (PlotCanvas) e la scrittura
 * dell'intestazione CSV (CsvLoggerController::start(), chiamato dal thread
 * GUI) lo leggono/scrivono tutti dal main thread. Il thread scrittore CSV
 * riceve invece una COPIA immutabile presa da start() (vedi
 * CsvLoggerController): nessun accesso concorrente, quindi nessun mutex.
 */
#pragma once

#include <array>
#include <string>

#include "Version.h"

namespace am {

struct ChannelCalibration {
    double a = 1.0;       ///< Coefficiente moltiplicativo.
    double b = 0.0;       ///< Offset additivo.
    std::string unit = "V";  ///< Unità di misura della grandezza convertita.
    std::string label;    ///< Descrizione libera (es. "Temperatura"); vuota =
                           ///< nessuna descrizione, usata come titolo della
                           ///< scheda dedicata al canale nel grafico (con
                           ///< fallback a "A0".."A5" se vuota).

    /// G = a*volts + b. Con i valori di default (a=1, b=0, unit="V") la
    /// conversione è l'identità: un canale non configurato mostra/registra
    /// esattamente la tensione in Volt, senza casi speciali da gestire altrove.
    [[nodiscard]] double convert(double volts) const { return a * volts + b; }
};

using ChannelCalibrations = std::array<ChannelCalibration, kNumAnalogChannels>;

} // namespace am
