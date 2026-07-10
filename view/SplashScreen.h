/**
 * @file SplashScreen.h
 * @brief Splash screen mostrato all'avvio dell'applicazione.
 *
 * VIEW (MVC): nessuna dipendenza da Model/Controller nel disegno o nella
 * creazione. Resta visibile fino alla prima connessione riuscita al
 * firmware Arduino (è MainController a chiamare Close() su di essa, dal suo
 * gestore di EVT_CONNECTION_CHANGED — la View non conosce il Controller, ma
 * il Controller può agire su un puntatore a finestra come già fa altrove,
 * es. su MainFrame). @p safetyTimeoutMs è una rete di sicurezza: se nessuna
 * scheda si connette entro quel tempo, la splash si chiude comunque da sola
 * per non bloccare l'avvio dell'app.
 */
#pragma once

#include <wx/splash.h>

namespace am {

/// Crea e mostra lo splash screen di avvio. Va invocato da wxApp::OnInit
/// PRIMA di costruire la finestra principale, seguito da un wxYield() per
/// forzare il primo repaint (OnInit gira prima che il loop eventi sia
/// avviato). Il puntatore restituito va passato a
/// MainController::initialize() perché possa chiuderla alla connessione.
wxSplashScreen* showSplashScreen(int safetyTimeoutMs = 20000);

} // namespace am
