/**
 * @file main.cpp
 * @brief Punto di ingresso: wxApp che possiede il MainController.
 *
 * L'applicazione (wxApp) crea il MainController, che a sua volta possiede
 * il Model, gli altri Controller e la finestra principale (View).
 * Nessuna variabile globale: tutto lo stato vive nel controller, il cui
 * ciclo di vita è legato (RAII) a quello dell'applicazione.
 */
#include <memory>

#include <wx/app.h>
#include <wx/image.h>

#include "controller/MainController.h"
#include "view/SplashScreen.h"

namespace am {

class AliveMonitorApp : public wxApp {
public:
    bool OnInit() override
    {
        if (!wxApp::OnInit()) {
            return false;
        }
        // Handler immagini: necessario per l'esportazione PNG del grafico.
        wxInitAllImageHandlers();

        // Splash screen: resta visibile finché MainController non la chiude
        // (prima connessione riuscita al firmware), con un timeout di
        // sicurezza se nessuna scheda si connette. wxYield() forza il primo
        // repaint perché OnInit gira prima che il loop eventi sia avviato.
        wxSplashScreen* splash = showSplashScreen();
        wxYield();

        controller_ = std::make_unique<MainController>();
        controller_->initialize(splash);
        return true;
    }

    int OnExit() override
    {
        // Chiusura ordinata anche in percorsi di uscita anomali
        // (shutdown() è idempotente: se già chiamato non fa nulla).
        if (controller_) {
            controller_->shutdown();
            controller_.reset();
        }
        return wxApp::OnExit();
    }

private:
    std::unique_ptr<MainController> controller_;
};

} // namespace am

// Genera il main()/WinMain() appropriato per la piattaforma.
wxIMPLEMENT_APP(am::AliveMonitorApp);
