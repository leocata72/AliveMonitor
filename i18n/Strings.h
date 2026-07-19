/**
 * @file Strings.h
 * @brief Tabella di traduzione dell'interfaccia (IT/EN/FR/ES/DE).
 *
 * Approccio scelto DELIBERATAMENTE al posto del meccanismo i18n nativo di
 * wxWidgets (wxLocale + cataloghi gettext .po/.mo): una tabella di stringhe
 * costante in C++, indicizzata da un enum StringId, letta con tr(id). Zero
 * dipendenze e strumenti di build aggiuntivi (niente msgfmt, niente file
 * .mo da distribuire/installare accanto all'eseguibile o nel setup.exe),
 * coerente con lo stile "nessuna libreria esterna" già seguito nel resto
 * del progetto (renderer del grafico custom invece di wxMathPlot, ecc.).
 *
 * Accesso da più thread: la lingua corrente è un std::atomic<Language> e la
 * TABELLA stessa è dato costante (mai mutata a runtime) — esattamente come
 * altrove nel progetto un flag std::atomic<bool> permette letture sicure da
 * più thread senza mutex (vedi CsvLoggerController::active_). Questo è
 * importante perché alcuni messaggi (es. motivo di una disconnessione)
 * vengono formattati nel thread seriale (SerialController), non solo nel
 * thread GUI: tr() è sicura da chiamare da entrambi.
 *
 * Stringhe NON tradotte deliberatamente: unità di misura e sigle tecniche
 * standard identiche in tutte le lingue supportate (V, Hz, ms, °C, FPS, RX,
 * CRC, CPU, CSV, PNG) — tradurle risulterebbe innaturale (nessun software
 * tecnico traduce "FPS" in francese o tedesco) — e il protocollo seriale
 * (HELLO, ARDUINO_UNO, comandi GET/SET...), che è un contratto col firmware
 * e non deve mai cambiare in base alla lingua dell'interfaccia.
 */
#pragma once

#include <wx/string.h>

#include "Language.h"

namespace am {

enum class StringId {
    // --- ToolbarPanel --------------------------------------------------------
    TbSearching,
    TbConnected,
    TbDisconnected,
    TbPortPrefix,
    TbFwPrefix,
    TbConnectBtn,
    TbDisconnectBtn,

    // --- DigitalOutputPanel ----------------------------------------------------
    DoBoxTitle,
    DoOn,
    DoOff,
    DoDirTooltip,    ///< tooltip della wxChoice IN/OUT (v1.2)
    DoTimedCheck,    ///< "Temporizzato" (v1.2)
    DoTimeOnLabel,   ///< "ON (s):"
    DoPeriodLabel,   ///< "Periodo (s):"
    DoTimedTooltip,  ///< spiega il ciclo e il valore speciale "inf"

    // --- AcquisitionPanel --------------------------------------------------------
    AqBoxTitle,
    AqStart,
    AqPause,
    AqStop,
    AqCsvCheck,
    AqCsvTooltip,
    AqFreqLabel,
    AqSetRateFmt,        ///< "Impostata: %d Hz (conferma FW: %s)"
    AqMeasuredRateFmt,   ///< "Misurata: %.1f Hz"
    AqMeasuredRateNone,  ///< "Misurata: - Hz"
    AqSamplePeriodFmt,   ///< "Campionamento: %.2f ms"
    AqSamplePeriodNone,  ///< "Campionamento: - ms"

    // --- StatusPanel -----------------------------------------------------------
    StAcqFmt,        ///< "Acq: %d Hz (eff. %.1f)"
    StAcqFmtNoEff,   ///< "Acq: %d Hz"
    StLostFmt,       ///< "Persi: %llu"
    StErrFmt,        ///< "Err: %llu"
    StConnPrefix,    ///< "Conn: "
    StCpuFmt,        ///< "CPU: %.1f%%"
    StCpuNone,       ///< "CPU: n/d"

    // --- SettingsDialog ----------------------------------------------------------
    SdTitle,
    SdFpsLabel,
    SdWindowLabel,
    SdAutoYLabel,
    SdLanguageLabel,
    SdRestartNotice,  ///< avviso che il cambio lingua richiede il riavvio

    // --- SplashScreen ------------------------------------------------------------
    SpSubtitle,
    SpVersionFmt,   ///< "Version %s"
    SpCreditsFmt,   ///< "by: %s"  (i nomi non si traducono)

    // --- CalibrationPanel --------------------------------------------------------
    CalBoxTitle,
    CalColUnit,
    CalColDescription,
    CalColMarker,   ///< intestazione colonna marker (v1.2)
    CalBtnSave,     ///< "Salva..." configurazione canali su file
    CalBtnLoad,     ///< "Carica..." configurazione canali da file

    // --- Marker dei campioni (ordine = enum MarkerStyle) --------------------------
    MkNone,
    MkCircleFull,
    MkCircleOpen,
    MkSquare,
    MkTriangle,
    MkDiamond,
    MkStar,
    MkCross,
    MkX,

    // --- GraphPanel ----------------------------------------------------------------
    GpTabAll,
    GpAxisTime,
    GpCsvNone,
    GpCsvPrefix,
    GpAutoYCheck,
    GpBtnAutoscale,
    GpBtnFollow,
    GpBtnReset,
    GpYMinLabel,    ///< "Y min" (v1.2)
    GpYMaxLabel,    ///< "Y max"
    GpYStepLabel,   ///< "Passo" delle tacche Y

    // --- MainFrame: menu -----------------------------------------------------------
    MfMenuFile,
    MfMenuExportPng,
    MfMenuExportPngHelp,
    MfMenuExit,
    MfMenuTools,
    MfMenuSettings,
    MfMenuHelp,
    MfMenuGuide,
    MfMenuAbout,
    MfGuideWriteError,
    MfAboutDescription,
    MfAboutLicence,

    // --- OptionsPanel (riquadro "Opzioni", v1.2) -----------------------------------
    OpBoxTitle,
    OpSimultaneousTimers,
    OpSimultaneousTimersTooltip,
    OpStatsSamplesLabel,    ///< "Campioni statistiche:" (spin N campioni)
    OpStatsSamplesTooltip,
    OpStatsColMean,         ///< intestazione colonna "media" della griglia
                            ///< statistiche (σ/max/min sono notazione
                            ///< universale, non tradotti)

    // --- MainController ------------------------------------------------------------
    McSaveCsvDialogTitle,
    McCsvOpenError,
    McExportPngDialogTitle,
    McExportPngError,
    McCsvFilter,
    McPngFilter,
    McErrPrefix,
    McSaveChCfgTitle,   ///< dialogo salvataggio configurazione canali (v1.2)
    McLoadChCfgTitle,   ///< dialogo caricamento configurazione canali
    McChCfgFilter,      ///< filtro "File di testo (*.txt)"
    McChCfgSaveError,
    McChCfgLoadError,

    // --- SerialController (thread seriale: vedi nota thread-safety sopra) -----------
    ScDisconnectedByUser,
    ScSearching,
    ScIoError,
    ScTimeout,
    ScWriteError,
};

/// Imposta la lingua corrente (thread-safe, effetto immediato su tr()).
void setLanguage(Language lang);

/// Lingua correntemente attiva.
[[nodiscard]] Language currentLanguage();

/// Traduce id nella lingua corrente. Le stringhe con placeholder (%d, %s,
/// %.1f...) vanno passate a wxString::Format() dal chiamante, esattamente
/// come già si faceva con i literal prima di questo refactor.
[[nodiscard]] wxString tr(StringId id);

/// Nome nativo della lingua (per il selettore in SettingsDialog: "Italiano",
/// "English", "Français", "Español", "Deutsch" — sempre nella lingua stessa,
/// non tradotti, così restano leggibili a chi non capisce la lingua corrente).
[[nodiscard]] wxString nativeLanguageName(Language lang);

} // namespace am
