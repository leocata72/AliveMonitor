/**
 * @file IUserActions.h
 * @brief Interfaccia delle azioni utente (View -> Controller).
 *
 * Le View NON conoscono i Controller concreti: emettono le azioni dell'utente
 * attraverso questa interfaccia astratta, implementata da MainController.
 * Questo disaccoppia completamente la View dalla logica applicativa e rende
 * le View testabili con un mock di IUserActions.
 */
#pragma once

#include <wx/string.h>

namespace am {

class IUserActions {
public:
    virtual ~IUserActions() = default;

    // --- Connessione -------------------------------------------------------
    /// L'utente ha premuto "Connetti": riabilita la ricerca automatica.
    virtual void onConnectRequested() = 0;
    /// L'utente ha premuto "Disconnetti": chiude e sospende la ricerca.
    virtual void onDisconnectRequested() = 0;

    // --- Uscite digitali ---------------------------------------------------
    /// Toggle di un'uscita digitale (pin 2..9).
    virtual void onDigitalOutputToggled(int pin, bool on) = 0;

    /// Avvio di un ciclo temporizzato sull'uscita (pulsante ON premuto con
    /// "Temporizzato" attivo). @param oneShot true = un solo impulso ON di
    /// onSeconds e poi OFF definitivo (campo Periodo = "inf" nella GUI).
    virtual void onTimedOutputStarted(int pin, double onSeconds,
                                      double periodSeconds, bool oneShot) = 0;
    /// Interruzione manuale del ciclo temporizzato (pulsante su OFF o
    /// checkbox "Temporizzato" tolto a ciclo in corso): spegne subito.
    virtual void onTimedOutputStopped(int pin) = 0;

    /// Opzione "Avvio contemporaneo temporizzatori" (riquadro Opzioni):
    /// se attiva, il primo ON su un'uscita temporizzata avvia anche tutte
    /// le altre uscite con "Temporizzato" spuntato e campi validi.
    virtual void onSimultaneousTimersChanged(bool enabled) = 0;

    // --- Acquisizione ------------------------------------------------------
    /// Avvia (o riprende da Pausa) l'acquisizione. Se si tratta di una NUOVA
    /// sessione (da Stopped), il Controller chiede prima il file dove
    /// registrare il CSV: se l'utente annulla, l'acquisizione non parte.
    virtual void onAcquisitionStart() = 0;
    virtual void onAcquisitionStop()  = 0;
    virtual void onAcquisitionPause() = 0;
    /// Nuova frequenza richiesta dall'utente [Hz]; valida anche in acquisizione.
    virtual void onSampleRateChanged(int rateHz) = 0;

    // --- Calibrazione canali -------------------------------------------------
    /// L'utente ha modificato la calibrazione (G = a*V + b, unità, descrizione)
    /// di un canale A0..A5 nella griglia. Effetto immediato su legenda del
    /// grafico (letta live) e sul titolo della scheda dedicata al canale
    /// (aggiornato esplicitamente, vedi GraphPanel::setChannelTabTitle); per
    /// il CSV vale solo la registrazione avviata DOPO questa modifica (il
    /// file già aperto non cambia intestazione a metà).
    /// @param label descrizione libera (es. "Temperatura"), può essere vuota.
    virtual void onCalibrationChanged(int channel, double a, double b,
                                      const wxString& unit,
                                      const wxString& label) = 0;

    /// L'utente ha scelto un marker per il canale nella griglia.
    /// @param markerIndex indice nell'enum MarkerStyle (0 = nessuno).
    virtual void onChannelMarkerChanged(int channel, int markerIndex) = 0;

    /// Salvataggio/caricamento della configurazione dei canali (a, b, unità,
    /// nome) su/da file di testo: il Controller mostra il dialogo file e
    /// gestisce l'I/O (vedi ChannelConfigFile).
    virtual void onChannelConfigSaveRequested() = 0;
    virtual void onChannelConfigLoadRequested() = 0;

    // --- Esportazione / impostazioni ----------------------------------------
    virtual void onExportPngRequested() = 0;
    virtual void onSettingsRequested()  = 0;

    // --- Ciclo di vita -------------------------------------------------------
    /// Chiusura ordinata: ferma timer e thread prima della distruzione della GUI.
    virtual void onShutdown() = 0;
};

} // namespace am
