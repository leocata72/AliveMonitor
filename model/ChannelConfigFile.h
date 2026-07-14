/**
 * @file ChannelConfigFile.h
 * @brief Salvataggio/caricamento su file di testo della configurazione dei
 *        canali analogici (a, b, unità, nome per ciascun canale A0..A5).
 *
 * MODEL (MVC). Formato deliberatamente leggibile e modificabile a mano
 * (stile INI, UTF-8, decimali col PUNTO indipendentemente dalla locale):
 *
 *   # AliveMonitor - configurazione canali analogici (v1)
 *   [A0]
 *   a=1
 *   b=0
 *   unit=V
 *   label=Temperatura
 *   marker=6
 *   ...
 *
 * Il parser è tollerante: righe vuote e commenti (#, ;) ignorati, sezioni o
 * chiavi mancanti lasciano il canale corrispondente INVARIATO (load() parte
 * dalla configurazione corrente e sovrascrive solo ciò che trova nel file).
 * marker è l'indice numerico dell'enum MarkerStyle (0 = nessuno): un file
 * salvato da una versione precedente, che non ha la chiave, carica senza
 * errori lasciando il marker corrente com'è.
 */
#pragma once

#include <string>

#include "model/ChannelCalibration.h"

namespace am {

class ChannelConfigFile {
public:
    ChannelConfigFile() = delete;  ///< Solo metodi statici.

    /// Scrive la configurazione su file (sovrascrivendolo).
    /// @return false se il file non può essere aperto o la scrittura fallisce.
    [[nodiscard]] static bool save(const std::string& path,
                                   const ChannelCalibrations& calibrations);

    /// Legge il file e aggiorna IN PLACE i campi trovati (a, b, unit, label,
    /// marker); i campi assenti nel file restano invariati.
    /// @return false se il file non può essere aperto o non contiene alcuna
    ///         sezione canale riconosciuta (file non valido).
    [[nodiscard]] static bool load(const std::string& path,
                                   ChannelCalibrations& calibrations);
};

} // namespace am
