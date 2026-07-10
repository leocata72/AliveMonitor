/**
 * @file MainFrame.cpp
 * @brief Implementazione della finestra principale.
 */
#include "view/MainFrame.h"

#include <wx/aboutdlg.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/sizer.h>

#include "Version.h"
#include "view/AcquisitionPanel.h"
#include "view/DigitalOutputPanel.h"
#include "view/GraphPanel.h"
#include "view/StatusPanel.h"
#include "view/ToolbarPanel.h"

// Icona dell'applicazione (formato XPM, incorporata come array C: nessun
// file esterno da distribuire insieme all'eseguibile).
#include "resources/AliveMonitor.xpm"

namespace am {
namespace {

// ID dei comandi di menu.
constexpr int kMenuExportPng = wxID_HIGHEST + 1;

} // namespace

MainFrame::MainFrame(IUserActions& actions,
                     BoardState& WXUNUSED(state),
                     DigitalOutputState& WXUNUSED(outputs),
                     const AnalogDataBuffer& buffer)
    : wxFrame(nullptr, wxID_ANY,
              wxString::Format("%s %s", kAppName, kAppVersion),
              wxDefaultPosition, wxSize(1200, 760))
    , actions_(actions)
{
    SetMinSize(wxSize(980, 620));
    SetIcon(wxIcon(AliveMonitor_xpm));  // icona di titolo/taskbar
    buildMenuBar();

    // --- Composizione del layout -------------------------------------------
    //  +--------------------------------------------------+
    //  | ToolbarPanel                                     |
    //  +----------------+---------------------------------+
    //  | DigitalOutput  |                                 |
    //  | Panel          |          GraphPanel             |
    //  | Acquisition    |                                 |
    //  | Panel          |                                 |
    //  +----------------+---------------------------------+
    //  | StatusPanel                                      |
    //  +--------------------------------------------------+
    auto* root = new wxBoxSizer(wxVERTICAL);

    toolbar_ = new ToolbarPanel(this, actions_);
    root->Add(toolbar_, 0, wxEXPAND | wxALL, 4);

    auto* middle = new wxBoxSizer(wxHORIZONTAL);

    auto* left = new wxBoxSizer(wxVERTICAL);
    digital_ = new DigitalOutputPanel(this, actions_);
    acquisition_ = new AcquisitionPanel(this, actions_);
    left->Add(digital_, 0, wxEXPAND);
    left->Add(acquisition_, 0, wxEXPAND);
    left->AddStretchSpacer(1);
    middle->Add(left, 0, wxEXPAND | wxLEFT, 4);

    graph_ = new GraphPanel(this, actions_, buffer);
    middle->Add(graph_, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);

    root->Add(middle, 1, wxEXPAND);

    status_ = new StatusPanel(this);
    root->Add(status_, 0, wxEXPAND | wxALL, 4);

    SetSizer(root);

    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::onClose, this);
}

void MainFrame::buildMenuBar()
{
    auto* fileMenu = new wxMenu();
    fileMenu->Append(kMenuExportPng, "Esporta grafico &PNG...\tCtrl+P",
                     "Salva il grafico corrente come immagine PNG");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "&Esci\tAlt+F4");

    auto* toolsMenu = new wxMenu();
    toolsMenu->Append(wxID_PREFERENCES, "&Impostazioni...\tCtrl+,");

    auto* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, "&Informazioni...");

    auto* bar = new wxMenuBar();
    bar->Append(fileMenu, "&File");
    bar->Append(toolsMenu, "&Strumenti");
    bar->Append(helpMenu, "&Aiuto");
    SetMenuBar(bar);

    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        actions_.onExportPngRequested();
    }, kMenuExportPng);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        actions_.onSettingsRequested();
    }, wxID_PREFERENCES);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        Close(false);
    }, wxID_EXIT);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxAboutDialogInfo info;
        info.SetName(kAppName);
        info.SetVersion(kAppVersion);
        info.SetIcon(wxIcon(AliveMonitor_xpm));
        info.SetDescription(
            "Acquisizione dati analogici e controllo uscite digitali\n"
            "di Arduino Uno via porta seriale.\n\n"
            "wxWidgets");
        info.AddDeveloper("Leonardo Catalano");
        info.AddDeveloper("Claude");
        wxAboutBox(info, this);
    }, wxID_ABOUT);
}

void MainFrame::onClose(wxCloseEvent& event)
{
    // Chiusura ordinata: prima si fermano thread e timer, poi si distrugge
    // la finestra. onShutdown() è idempotente.
    actions_.onShutdown();
    event.Skip();  // prosegue con la distruzione predefinita del frame
}

} // namespace am
