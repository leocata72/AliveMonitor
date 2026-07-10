/**
 * @file AppEvents.cpp
 * @brief Definizione (unica) dei tipi di evento dichiarati in AppEvents.h.
 */
#include "AppEvents.h"

namespace am::events {

wxDEFINE_EVENT(EVT_CONNECTION_CHANGED, wxThreadEvent);
wxDEFINE_EVENT(EVT_STATS_UPDATED, wxThreadEvent);
wxDEFINE_EVENT(EVT_OUTPUT_STATE_CHANGED, wxThreadEvent);
wxDEFINE_EVENT(EVT_FIRMWARE_INFO, wxThreadEvent);
wxDEFINE_EVENT(EVT_RATE_CONFIRMED, wxThreadEvent);
wxDEFINE_EVENT(EVT_PROTOCOL_ERROR, wxThreadEvent);

} // namespace am::events
