/**
 * @file AppSettings.h
 * @brief Persistenza su file delle impostazioni utente (per ora: la lingua).
 *
 * MODEL (MVC). Prima eccezione, in questo progetto, alla filosofia "tutto
 * solo di sessione" seguita finora per ogni altra impostazione (calibrazione
 * canali, FPS del grafico, finestra temporale, autoscale Y: nessuna di
 * queste viene salvata su disco, vanno reimpostate a ogni avvio). La lingua
 * dell'interfaccia fa eccezione perché, diversamente dalle altre, non è
 * legata a una sessione di misura ma è una preferenza stabile dell'utente:
 * dover riselezionare la lingua a ogni avvio sarebbe un fastidio evidente
 * senza alcun beneficio.
 *
 * Formato: file INI (wxFileConfig, modalità file locale esplicita, non
 * registro di Windows) in
 *   - Windows: %APPDATA%\AliveMonitor\settings.ini
 *   - Linux:   ~/.config/AliveMonitor/settings.ini
 * tramite wxStandardPaths::GetUserDataDir(), che richiede
 * wxApp::SetAppName(kAppName) sia già stato chiamato (fatto in main.cpp,
 * OnInit, prima di ogni altra cosa).
 *
 * Nessuno stato interno: entrambi i metodi sono statici e senza side effect
 * oltre alla lettura/scrittura del file. Non serve un'istanza né un
 * puntatore condiviso fra i chiamanti.
 */
#pragma once

#include <wx/string.h>

#include "Language.h"

namespace am {

class AppSettings {
public:
    /// True se esiste già una preferenza di lingua salvata su file (file
    /// presente E chiave Language presente al suo interno). Usato da
    /// MainController per decidere se mostrare LanguageSelectDialog al
    /// primo avvio: restituisce false sia quando il file non esiste ancora,
    /// sia quando esiste ma non contiene (ancora) quella chiave, così in
    /// futuro altre impostazioni potranno condividere lo stesso file senza
    /// far scattare per errore la richiesta di lingua a un utente che
    /// l'aveva già scelta in una versione precedente priva di quella chiave.
    [[nodiscard]] static bool hasSavedLanguage();

    /// Legge la lingua salvata su file. Se il file non esiste, non contiene
    /// la chiave attesa, o il valore letto è fuori dai limiti validi (indice
    /// negativo o >= kLanguageCount), restituisce silenziosamente
    /// kDefaultLanguage: non è un errore, è il comportamento atteso al primo
    /// avvio dell'applicazione (nessun file ancora creato).
    [[nodiscard]] static Language loadLanguage();

    /// Salva la lingua scelta su file, creando cartella e file se mancanti.
    /// Eventuali errori di scrittura (percorso non accessibile) sono
    /// ignorati silenziosamente: nel caso peggiore la preferenza non verrà
    /// ricordata al prossimo avvio, ma l'applicazione resta pienamente
    /// funzionante nella sessione corrente (setLanguage() ha già effetto
    /// immediato indipendentemente dal risultato del salvataggio).
    static void saveLanguage(Language lang);

private:
    /// Percorso completo del file INI, sempre lo stesso per l'intero processo.
    [[nodiscard]] static wxString iniPath();
};

} // namespace am
