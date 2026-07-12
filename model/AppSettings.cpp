/**
 * @file AppSettings.cpp
 * @brief Implementazione della persistenza su file delle impostazioni utente.
 */
#include "model/AppSettings.h"

#include <wx/fileconf.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace am {
namespace {

constexpr const char* kIniFileName = "settings.ini";
constexpr const char* kLanguageKey = "/Language";

} // namespace

wxString AppSettings::iniPath()
{
    const wxString dir = wxStandardPaths::Get().GetUserDataDir();
    return dir + wxFileName::GetPathSeparator() + kIniFileName;
}

bool AppSettings::hasSavedLanguage()
{
    // wxFileExists prima di tutto: aprire un wxFileConfig su un percorso
    // inesistente non è un errore (lo tratta come vuoto), quindi da solo
    // HasEntry() non basterebbe a distinguere "file assente" da "file
    // presente ma senza quella chiave" — in entrambi i casi restituirebbe
    // false, il che va comunque bene qui, ma il controllo esplicito rende
    // l'intento leggibile ed evita di aprire inutilmente un file inesistente.
    if (!wxFileExists(iniPath())) {
        return false;
    }
    wxFileConfig config(wxEmptyString, wxEmptyString, iniPath(), wxEmptyString,
                         wxCONFIG_USE_LOCAL_FILE);
    return config.HasEntry(kLanguageKey);
}

Language AppSettings::loadLanguage()
{
    // wxCONFIG_USE_LOCAL_FILE: file INI esplicito, non il registro di
    // Windows (che sarebbe il default di wxFileConfig su quella piattaforma).
    // Se il file non esiste ancora, wxFileConfig lo tratta semplicemente come
    // vuoto: Read() sotto restituirà il valore di default richiesto.
    wxFileConfig config(wxEmptyString, wxEmptyString, iniPath(), wxEmptyString,
                         wxCONFIG_USE_LOCAL_FILE);

    long value = static_cast<long>(kDefaultLanguage);
    config.Read(kLanguageKey, &value, static_cast<long>(kDefaultLanguage));

    if (value < 0 || value >= kLanguageCount) {
        return kDefaultLanguage;  // valore corrotto/da versione futura: ignora
    }
    return static_cast<Language>(value);
}

void AppSettings::saveLanguage(Language lang)
{
    const wxString path = iniPath();

    // Il file può trovarsi in una cartella (%APPDATA%\AliveMonitor) non
    // ancora creata al primo salvataggio: wxFileConfig non la crea da sé.
    const wxString dir = wxFileName(path).GetPath();
    if (!wxFileName::DirExists(dir)) {
        wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }

    wxFileConfig config(wxEmptyString, wxEmptyString, path, wxEmptyString,
                         wxCONFIG_USE_LOCAL_FILE);
    config.Write(kLanguageKey, static_cast<long>(lang));
    config.Flush();
}

} // namespace am
