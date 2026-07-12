/**
 * @file Strings.cpp
 * @brief Tabella di traduzione IT/EN/FR/ES/DE (vedi Strings.h per il design).
 */
#include "i18n/Strings.h"

#include <atomic>

namespace am {
namespace {

// Lingua corrente: atomica per essere leggibile senza lock anche dal thread
// seriale (SerialController posta messaggi come "Nessuna risposta da
// Arduino" già tradotti). La tabella sottostante è dato costante, mai
// mutata a runtime: le uniche due condizioni per una lettura concorrente
// sicura senza mutex sono entrambe soddisfatte.
std::atomic<Language> gCurrentLanguage{ kDefaultLanguage };

/// Le cinque traduzioni di una singola stringa, in ordine IT/EN/FR/ES/DE
/// (stesso ordine dei valori dell'enum Language).
struct Entry {
    const char* it;
    const char* en;
    const char* fr;
    const char* es;
    const char* de;
};

const char* pick(const Entry& e, Language lang)
{
    switch (lang) {
    case Language::IT: return e.it;
    case Language::EN: return e.en;
    case Language::FR: return e.fr;
    case Language::ES: return e.es;
    case Language::DE: return e.de;
    }
    return e.it;  // irraggiungibile: switch esaustivo su tutti i valori dell'enum
}

} // namespace

void setLanguage(Language lang)
{
    gCurrentLanguage.store(lang, std::memory_order_relaxed);
}

Language currentLanguage()
{
    return gCurrentLanguage.load(std::memory_order_relaxed);
}

wxString tr(StringId id)
{
    // Uno switch (non una tabella indicizzata da static_cast<int>(id)) è
    // deliberato: con -Wall/-Wextra (GCC/Clang) e /W4 (MSVC) il compilatore
    // segnala in build un case StringId dimenticato, cosa che un array
    // parallelo all'enum non garantirebbe (basterebbe un elemento in meno o
    // in ordine sbagliato per disallineare silenziosamente tutta la tabella
    // dopo quel punto).
    Entry e{};
    switch (id) {
    // --- ToolbarPanel --------------------------------------------------------
    case StringId::TbSearching:
        e = { "Ricerca...", "Searching...", "Recherche...", "Buscando...", "Suche..." };
        break;
    case StringId::TbConnected:
        e = { "Connesso", "Connected", "Connecté", "Conectado", "Verbunden" };
        break;
    case StringId::TbDisconnected:
        e = { "Disconnesso", "Disconnected", "Déconnecté", "Desconectado", "Getrennt" };
        break;
    case StringId::TbPortPrefix:
        e = { "Porta: ", "Port: ", "Port : ", "Puerto: ", "Port: " };
        break;
    case StringId::TbFwPrefix:
        e = { "FW: ", "FW: ", "FW : ", "FW: ", "FW: " };
        break;
    case StringId::TbConnectBtn:
        e = { "Connetti", "Connect", "Connecter", "Conectar", "Verbinden" };
        break;
    case StringId::TbDisconnectBtn:
        e = { "Disconnetti", "Disconnect", "Déconnecter", "Desconectar", "Trennen" };
        break;

    // --- DigitalOutputPanel ----------------------------------------------------
    case StringId::DoBoxTitle:
        e = { "Uscite digitali", "Digital outputs", "Sorties numériques",
              "Salidas digitales", "Digitale Ausgänge" };
        break;
    case StringId::DoOn:
        e = { "ON", "ON", "ON", "ON", "ON" };
        break;
    case StringId::DoOff:
        e = { "OFF", "OFF", "OFF", "OFF", "OFF" };
        break;

    // --- AcquisitionPanel --------------------------------------------------------
    case StringId::AqBoxTitle:
        e = { "Acquisizione", "Acquisition", "Acquisition", "Adquisición", "Erfassung" };
        break;
    case StringId::AqStart:
        e = { "Start", "Start", "Démarrer", "Iniciar", "Start" };
        break;
    case StringId::AqPause:
        e = { "Pause", "Pause", "Pause", "Pausa", "Pause" };
        break;
    case StringId::AqStop:
        e = { "Stop", "Stop", "Arrêter", "Detener", "Stopp" };
        break;
    case StringId::AqCsvCheck:
        e = { "Registra CSV", "Record CSV", "Enregistrer CSV", "Registrar CSV",
              "CSV aufzeichnen" };
        break;
    case StringId::AqCsvTooltip:
        e = { "Se spuntata, Start chiede dove salvare la registrazione CSV. "
              "Se smarcata, Start avvia l'acquisizione senza chiedere né scrivere alcun file.",
              "If checked, Start asks where to save the CSV recording. "
              "If unchecked, Start begins acquisition immediately, without asking or writing any file.",
              "Si cochée, Démarrer demande où enregistrer le fichier CSV. "
              "Si décochée, l'acquisition démarre immédiatement, sans demander ni écrire aucun fichier.",
              "Si está marcada, Iniciar pregunta dónde guardar el registro CSV. "
              "Si no lo está, la adquisición comienza de inmediato, sin preguntar ni escribir ningún archivo.",
              "Falls aktiviert, fragt Start, wo die CSV-Aufzeichnung gespeichert werden soll. "
              "Falls deaktiviert, beginnt die Erfassung sofort, ohne Nachfrage und ohne eine Datei zu schreiben." };
        break;
    case StringId::AqFreqLabel:
        e = { "Frequenza [Hz]:", "Frequency [Hz]:", "Fréquence [Hz] :",
              "Frecuencia [Hz]:", "Frequenz [Hz]:" };
        break;
    case StringId::AqSetRateFmt:
        e = { "Impostata: %d Hz (conferma FW: %s)", "Set: %d Hz (FW confirms: %s)",
              "Réglée : %d Hz (confirmation FW : %s)", "Ajustada: %d Hz (confirmación FW: %s)",
              "Eingestellt: %d Hz (FW-Bestätigung: %s)" };
        break;
    case StringId::AqMeasuredRateFmt:
        e = { "Misurata: %.1f Hz", "Measured: %.1f Hz", "Mesurée : %.1f Hz",
              "Medida: %.1f Hz", "Gemessen: %.1f Hz" };
        break;
    case StringId::AqMeasuredRateNone:
        e = { "Misurata: - Hz", "Measured: - Hz", "Mesurée : - Hz",
              "Medida: - Hz", "Gemessen: - Hz" };
        break;
    case StringId::AqSamplePeriodFmt:
        e = { "Campionamento: %.2f ms", "Sampling: %.2f ms", "Échantillonnage : %.2f ms",
              "Muestreo: %.2f ms", "Abtastung: %.2f ms" };
        break;
    case StringId::AqSamplePeriodNone:
        e = { "Campionamento: - ms", "Sampling: - ms", "Échantillonnage : - ms",
              "Muestreo: - ms", "Abtastung: - ms" };
        break;

    // --- StatusPanel -----------------------------------------------------------
    case StringId::StAcqFmt:
        e = { "Acq: %d Hz (eff. %.1f)", "Acq: %d Hz (eff. %.1f)", "Acq : %d Hz (eff. %.1f)",
              "Adq: %d Hz (ef. %.1f)", "Erf.: %d Hz (eff. %.1f)" };
        break;
    case StringId::StAcqFmtNoEff:
        e = { "Acq: %d Hz", "Acq: %d Hz", "Acq : %d Hz", "Adq: %d Hz", "Erf.: %d Hz" };
        break;
    case StringId::StLostFmt:
        e = { "Persi: %llu", "Lost: %llu", "Perdus : %llu", "Perdidos: %llu", "Verloren: %llu" };
        break;
    case StringId::StErrFmt:
        e = { "Err: %llu", "Err: %llu", "Err : %llu", "Err: %llu", "Fehl.: %llu" };
        break;
    case StringId::StConnPrefix:
        e = { "Conn: ", "Conn: ", "Conn : ", "Con: ", "Verb.: " };
        break;
    case StringId::StCpuFmt:
        e = { "CPU: %.1f%%", "CPU: %.1f%%", "CPU : %.1f%%", "CPU: %.1f%%", "CPU: %.1f%%" };
        break;
    case StringId::StCpuNone:
        e = { "CPU: n/d", "CPU: n/a", "CPU : n/d", "CPU: n/d", "CPU: k. A." };
        break;

    // --- SettingsDialog ----------------------------------------------------------
    case StringId::SdTitle:
        e = { "Impostazioni", "Settings", "Paramètres", "Configuración", "Einstellungen" };
        break;
    case StringId::SdFpsLabel:
        e = { "FPS del grafico:", "Graph FPS:", "FPS du graphique :",
              "FPS del gráfico:", "Diagramm-FPS:" };
        break;
    case StringId::SdWindowLabel:
        e = { "Finestra temporale [s]:", "Time window [s]:", "Fenêtre temporelle [s] :",
              "Ventana temporal [s]:", "Zeitfenster [s]:" };
        break;
    case StringId::SdAutoYLabel:
        e = { "Autoscale Y continuo:", "Continuous Y autoscale:", "Autoéchelle Y continue :",
              "Autoescala Y continua:", "Fortlaufende Y-Autoskalierung:" };
        break;
    case StringId::SdLanguageLabel:
        e = { "Lingua:", "Language:", "Langue :", "Idioma:", "Sprache:" };
        break;
    case StringId::SdRestartNotice:
        e = { "Riavvia AliveMonitor per applicare la nuova lingua.",
              "Restart AliveMonitor to apply the new language.",
              "Redémarrez AliveMonitor pour appliquer la nouvelle langue.",
              "Reinicia AliveMonitor para aplicar el nuevo idioma.",
              "Starten Sie AliveMonitor neu, um die neue Sprache zu übernehmen." };
        break;

    // --- SplashScreen ------------------------------------------------------------
    // Sottotitolo/versione/crediti erano già in inglese anche nella build
    // italiana originale (scelta stilistica del "classic UI" della splash,
    // non testo funzionale): restano identici in tutte le lingue invece di
    // essere forzatamente tradotti.
    case StringId::SpSubtitle:
        e = { "Simple Control Software", "Simple Control Software", "Simple Control Software",
              "Simple Control Software", "Simple Control Software" };
        break;
    case StringId::SpVersionFmt:
        e = { "Version %s", "Version %s", "Version %s", "Version %s", "Version %s" };
        break;
    case StringId::SpCreditsFmt:
        e = { "by: %s", "by: %s", "by: %s", "by: %s", "by: %s" };
        break;

    // --- CalibrationPanel --------------------------------------------------------
    case StringId::CalBoxTitle:
        e = { "Calibrazione (G = a·V + b)", "Calibration (G = a·V + b)",
              "Étalonnage (G = a·V + b)", "Calibración (G = a·V + b)",
              "Kalibrierung (G = a·V + b)" };
        break;
    case StringId::CalColUnit:
        e = { "unità", "unit", "unité", "unidad", "Einheit" };
        break;
    case StringId::CalColDescription:
        e = { "descrizione", "description", "description", "descripción", "Beschreibung" };
        break;

    // --- GraphPanel ----------------------------------------------------------------
    case StringId::GpTabAll:
        e = { "Tutti", "All", "Tous", "Todos", "Alle" };
        break;
    case StringId::GpAxisTime:
        e = { "Tempo [s]", "Time [s]", "Temps [s]", "Tiempo [s]", "Zeit [s]" };
        break;
    case StringId::GpCsvNone:
        e = { "CSV: nessuna registrazione", "CSV: no recording", "CSV : aucun enregistrement",
              "CSV: sin registro", "CSV: keine Aufzeichnung" };
        break;
    case StringId::GpCsvPrefix:
        e = { "CSV: ", "CSV: ", "CSV : ", "CSV: ", "CSV: " };
        break;
    case StringId::GpAutoYCheck:
        e = { "Auto Y", "Auto Y", "Auto Y", "Auto Y", "Auto Y" };
        break;
    case StringId::GpBtnAutoscale:
        e = { "Autoscala", "Autoscale", "Auto-échelle", "Autoescala", "Autoskalierung" };
        break;
    case StringId::GpBtnFollow:
        e = { "Segui", "Follow", "Suivre", "Seguir", "Folgen" };
        break;
    case StringId::GpBtnReset:
        e = { "Reset", "Reset", "Réinitialiser", "Restablecer", "Zurücksetzen" };
        break;

    // --- MainFrame: menu -----------------------------------------------------------
    case StringId::MfMenuFile:
        e = { "&File", "&File", "&Fichier", "&Archivo", "&Datei" };
        break;
    case StringId::MfMenuExportPng:
        e = { "Esporta grafico &PNG...\tCtrl+P", "Export graph as &PNG...\tCtrl+P",
              "Exporter le graphique en &PNG...\tCtrl+P", "Exportar gráfico como &PNG...\tCtrl+P",
              "Diagramm als &PNG exportieren...\tCtrl+P" };
        break;
    case StringId::MfMenuExportPngHelp:
        e = { "Salva il grafico corrente come immagine PNG",
              "Save the current graph as a PNG image",
              "Enregistre le graphique actuel au format PNG",
              "Guarda el gráfico actual como imagen PNG",
              "Speichert das aktuelle Diagramm als PNG-Bild" };
        break;
    case StringId::MfMenuExit:
        e = { "&Esci\tAlt+F4", "E&xit\tAlt+F4", "&Quitter\tAlt+F4", "&Salir\tAlt+F4",
              "&Beenden\tAlt+F4" };
        break;
    case StringId::MfMenuTools:
        e = { "&Strumenti", "&Tools", "&Outils", "&Herramientas", "&Extras" };
        break;
    case StringId::MfMenuSettings:
        e = { "&Impostazioni...\tCtrl+,", "&Settings...\tCtrl+,", "&Paramètres...\tCtrl+,",
              "&Configuración...\tCtrl+,", "&Einstellungen...\tCtrl+," };
        break;
    case StringId::MfMenuHelp:
        e = { "&Aiuto", "&Help", "&Aide", "A&yuda", "&Hilfe" };
        break;
    case StringId::MfMenuGuide:
        e = { "&Guida...\tF1", "&Guide...\tF1", "&Guide...\tF1", "&Guía...\tF1",
              "&Anleitung...\tF1" };
        break;
    case StringId::MfMenuAbout:
        e = { "&Informazioni...", "&About...", "À &propos...", "&Acerca de...",
              "Übe&r..." };
        break;
    case StringId::MfGuideWriteError:
        e = { "Impossibile scrivere il file temporaneo della guida.",
              "Unable to write the guide's temporary file.",
              "Impossible d'écrire le fichier temporaire du guide.",
              "No se pudo escribir el archivo temporal de la guía.",
              "Die temporäre Datei der Anleitung konnte nicht geschrieben werden." };
        break;
    case StringId::MfAboutDescription:
        e = { "Acquisizione dati analogici e controllo uscite digitali\n"
              "di Arduino Uno (o compatibile) via porta seriale.\n\nwxWidgets",
              "Analog data acquisition and digital output control\n"
              "for Arduino Uno (or compatible) via serial port.\n\nwxWidgets",
              "Acquisition de données analogiques et contrôle des sorties numériques\n"
              "d'un Arduino Uno (ou compatible) via port série.\n\nwxWidgets",
              "Adquisición de datos analógicos y control de salidas digitales\n"
              "de un Arduino Uno (o compatible) mediante puerto serie.\n\nwxWidgets",
              "Erfassung analoger Daten und Steuerung digitaler Ausgänge\n"
              "eines Arduino Uno (oder kompatibel) über die serielle Schnittstelle.\n\nwxWidgets" };
        break;
    case StringId::MfAboutLicence:
        e = { "Licenza MIT: uso, modifica e ridistribuzione libera anche commerciale, "
              "fornito \"così com'è\" senza alcuna garanzia né responsabilità degli autori. "
              "Testo completo nel file LICENSE.",
              "MIT License: free to use, modify and redistribute, including commercially, "
              "provided \"as is\" with no warranty and no liability for the authors. "
              "Full text in the LICENSE file.",
              "Licence MIT : utilisation, modification et redistribution libres, y compris "
              "commerciales, fourni \"tel quel\" sans aucune garantie ni responsabilité des "
              "auteurs. Texte complet dans le fichier LICENSE.",
              "Licencia MIT: uso, modificación y redistribución libres, incluso comerciales, "
              "proporcionado \"tal cual\" sin garantía ni responsabilidad de los autores. "
              "Texto completo en el archivo LICENSE.",
              "MIT-Lizenz: freie Nutzung, Änderung und Weitergabe, auch kommerziell, "
              "bereitgestellt \"wie besehen\" ohne jegliche Gewährleistung oder Haftung der "
              "Autoren. Vollständiger Text in der Datei LICENSE." };
        break;

    // --- MainController ------------------------------------------------------------
    case StringId::McSaveCsvDialogTitle:
        e = { "Salva registrazione CSV", "Save CSV recording", "Enregistrer le fichier CSV",
              "Guardar registro CSV", "CSV-Aufzeichnung speichern" };
        break;
    case StringId::McCsvOpenError:
        e = { "Impossibile aprire il file CSV per la scrittura.",
              "Unable to open the CSV file for writing.",
              "Impossible d'ouvrir le fichier CSV en écriture.",
              "No se pudo abrir el archivo CSV para escritura.",
              "Die CSV-Datei konnte nicht zum Schreiben geöffnet werden." };
        break;
    case StringId::McExportPngDialogTitle:
        e = { "Esporta grafico come PNG", "Export graph as PNG", "Exporter le graphique en PNG",
              "Exportar gráfico como PNG", "Diagramm als PNG exportieren" };
        break;
    case StringId::McExportPngError:
        e = { "Esportazione PNG non riuscita.", "PNG export failed.",
              "L'export PNG a échoué.", "La exportación a PNG falló.",
              "PNG-Export fehlgeschlagen." };
        break;
    case StringId::McCsvFilter:
        e = { "File CSV (*.csv)|*.csv", "CSV files (*.csv)|*.csv",
              "Fichiers CSV (*.csv)|*.csv", "Archivos CSV (*.csv)|*.csv",
              "CSV-Dateien (*.csv)|*.csv" };
        break;
    case StringId::McPngFilter:
        e = { "Immagine PNG (*.png)|*.png", "PNG image (*.png)|*.png",
              "Image PNG (*.png)|*.png", "Imagen PNG (*.png)|*.png",
              "PNG-Bild (*.png)|*.png" };
        break;
    case StringId::McErrPrefix:
        e = { "ERR: ", "ERR: ", "ERR : ", "ERR: ", "FEHLER: " };
        break;

    // --- SerialController (thread seriale) ------------------------------------------
    case StringId::ScDisconnectedByUser:
        e = { "Disconnesso dall'utente", "Disconnected by user",
              "Déconnecté par l'utilisateur", "Desconectado por el usuario",
              "Vom Benutzer getrennt" };
        break;
    case StringId::ScSearching:
        e = { "Ricerca di Arduino...", "Searching for Arduino...",
              "Recherche d'Arduino...", "Buscando Arduino...", "Suche nach Arduino..." };
        break;
    case StringId::ScIoError:
        e = { "Errore I/O sulla porta seriale", "I/O error on serial port",
              "Erreur d'E/S sur le port série", "Error de E/S en el puerto serie",
              "E/A-Fehler auf der seriellen Schnittstelle" };
        break;
    case StringId::ScTimeout:
        e = { "Nessuna risposta da Arduino (timeout)", "No response from Arduino (timeout)",
              "Aucune réponse d'Arduino (délai dépassé)", "Sin respuesta de Arduino (tiempo agotado)",
              "Keine Antwort von Arduino (Zeitüberschreitung)" };
        break;
    case StringId::ScWriteError:
        e = { "Errore di scrittura sulla porta seriale", "Write error on serial port",
              "Erreur d'écriture sur le port série", "Error de escritura en el puerto serie",
              "Schreibfehler auf der seriellen Schnittstelle" };
        break;
    }

    return wxString::FromUTF8(pick(e, currentLanguage()));
}

wxString nativeLanguageName(Language lang)
{
    switch (lang) {
    case Language::IT: return wxString::FromUTF8("Italiano");
    case Language::EN: return wxString::FromUTF8("English");
    case Language::FR: return wxString::FromUTF8("Français");
    case Language::ES: return wxString::FromUTF8("Español");
    case Language::DE: return wxString::FromUTF8("Deutsch");
    }
    return wxString::FromUTF8("Italiano");  // irraggiungibile: switch esaustivo
}

} // namespace am
