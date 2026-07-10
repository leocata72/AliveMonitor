/**
 * @file AppEvents.h
 * @brief Eventi personalizzati wxWidgets usati per la comunicazione
 *        thread seriale -> thread GUI.
 *
 * Il thread seriale non tocca MAI direttamente i widget: aggiorna il Model
 * (thread-safe) e poi accoda un wxThreadEvent con wxQueueEvent(). Il main
 * thread riceve l'evento e rilegge lo stato dal Model. Gli eventi sono quindi
 * semplici "notifiche": il payload pesante resta nel Model.
 */
#pragma once

#include <wx/event.h>

namespace am::events {

/// La connessione è cambiata (connesso / ricerca / disconnesso).
/// GetString() contiene un'eventuale descrizione (es. motivo disconnessione).
wxDECLARE_EVENT(EVT_CONNECTION_CHANGED, wxThreadEvent);

/// Statistiche aggiornate (pacchetti, errori, frequenza misurata). Throttled.
wxDECLARE_EVENT(EVT_STATS_UPDATED, wxThreadEvent);

/// Stato reale di un'uscita digitale confermato dal firmware.
/// GetInt() = numero pin (2..9), GetExtraLong() = 0/1.
wxDECLARE_EVENT(EVT_OUTPUT_STATE_CHANGED, wxThreadEvent);

/// Versione firmware ricevuta. GetString() = versione.
wxDECLARE_EVENT(EVT_FIRMWARE_INFO, wxThreadEvent);

/// Frequenza di campionamento confermata dal firmware. GetInt() = Hz.
wxDECLARE_EVENT(EVT_RATE_CONFIRMED, wxThreadEvent);

/// Il firmware ha risposto ERR oppure è arrivata una riga non valida.
/// GetString() = descrizione.
wxDECLARE_EVENT(EVT_PROTOCOL_ERROR, wxThreadEvent);

} // namespace am::events
