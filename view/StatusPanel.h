/**
 * @file StatusPanel.h
 * @brief Vista di stato inferiore: adapter sulla wxStatusBar nativa del frame.
 *
 * VIEW (MVC). Fino alla 1.1 era un wxPanel con etichette separate da "|";
 * dalla 1.2 scrive invece nei campi di una vera wxStatusBar (creata da
 * MainFrame con CreateStatusBar()): aspetto nativo della piattaforma,
 * separatori e size-grip inclusi. La classe NON è più una finestra: è un
 * piccolo adapter senza ownership (la status bar appartiene al frame, come
 * ogni wxStatusBar). L'interfaccia update() resta identica, così
 * MainController non cambia.
 */
#pragma once

#include <array>
#include <optional>

#include <wx/string.h>

#include "model/BoardState.h"

class wxStatusBar;

namespace am {

class StatusPanel {
public:
    /// Numero di campi della status bar (per la CreateStatusBar del frame).
    static constexpr int kFieldCount = 8;

    /// @param bar status bar già creata dal frame (non posseduta). Imposta
    ///        larghezze dei campi e testi iniziali.
    explicit StatusPanel(wxStatusBar* bar);

    /// Aggiorna i campi (chiamata dal timer GUI a 2 Hz e sugli eventi).
    /// @param graphFps FPS del grafico misurati dal GraphController
    /// @param cpuUsage percentuale CPU del processo (nullopt = non disponibile)
    void update(const BoardState::Snapshot& snapshot,
                double graphFps,
                std::optional<double> cpuUsage);

private:
    /// Scrive il campo solo se il testo è cambiato: SetStatusText ridisegna
    /// sempre, e a 2 Hz il refresh incondizionato di 8 campi produrrebbe un
    /// tremolio visibile su alcune piattaforme.
    void setField(int field, const wxString& text);

    wxStatusBar* bar_;
    std::array<wxString, kFieldCount> last_;  ///< Ultimo testo per campo.
};

} // namespace am
