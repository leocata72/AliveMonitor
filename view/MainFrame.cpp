/**
 * @file MainFrame.cpp
 * @brief Implementazione della finestra principale.
 */
#include "view/MainFrame.h"

#include <cstring>

#include <wx/aboutdlg.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include "Version.h"
#include "i18n/Strings.h"
#include "view/AcquisitionPanel.h"
#include "view/CalibrationPanel.h"
#include "view/DigitalOutputPanel.h"
#include "view/GraphPanel.h"
#include "view/StatusPanel.h"
#include "view/ToolbarPanel.h"

// Icona dell'applicazione (formato XPM, incorporata come array C: nessun
// file esterno da distribuire insieme all'eseguibile).
#include "resources/AliveMonitor.xpm"
// Guida utente (HTML incorporato come stringa C, stesso principio dell'icona).
#include "resources/HelpContent.h"

namespace am {
namespace {

// ID dei comandi di menu.
constexpr int kMenuExportPng = wxID_HIGHEST + 1;
constexpr int kMenuHelp = wxID_HIGHEST + 2;

} // namespace

MainFrame::MainFrame(IUserActions& actions,
                     BoardState& WXUNUSED(state),
                     DigitalOutputState& WXUNUSED(outputs),
                     const AnalogDataBuffer& buffer,
                     const ChannelCalibrations& calibrations)
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
    calibration_ = new CalibrationPanel(this, actions_, calibrations);
    left->Add(digital_, 0, wxEXPAND);
    left->Add(acquisition_, 0, wxEXPAND);
    left->Add(calibration_, 0, wxEXPAND);
    left->AddStretchSpacer(1);
    middle->Add(left, 0, wxEXPAND | wxLEFT, 4);

    graph_ = new GraphPanel(this, actions_, buffer, calibrations);
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
    fileMenu->Append(kMenuExportPng, tr(StringId::MfMenuExportPng),
                     tr(StringId::MfMenuExportPngHelp));
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, tr(StringId::MfMenuExit));

    auto* toolsMenu = new wxMenu();
    toolsMenu->Append(wxID_PREFERENCES, tr(StringId::MfMenuSettings));

    auto* helpMenu = new wxMenu();
    helpMenu->Append(kMenuHelp, tr(StringId::MfMenuGuide));
    helpMenu->AppendSeparator();
    helpMenu->Append(wxID_ABOUT, tr(StringId::MfMenuAbout));

    auto* bar = new wxMenuBar();
    bar->Append(fileMenu, tr(StringId::MfMenuFile));
    bar->Append(toolsMenu, tr(StringId::MfMenuTools));
    bar->Append(helpMenu, tr(StringId::MfMenuHelp));
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
        // La guida (helpHtmlFor(), resources/HelpContent.h) è incorporata
        // nell'eseguibile come stringa, una variante per lingua: nessun file
        // da distribuire a parte. Va comunque scritta su disco una volta, in
        // una posizione temporanea, perché il browser di sistema possa
        // aprirla. currentLanguage() riflette la lingua attiva in questa
        // sessione (quella con cui l'interfaccia è stata costruita
        // all'avvio, non necessariamente quella salvata più di recente in
        // Impostazioni se non ancora applicata con un riavvio).
        const char* html = helpHtmlFor(currentLanguage());
        const wxString path = wxFileName(wxStandardPaths::Get().GetTempDir(),
                                         "AliveMonitor_Guida.html").GetFullPath();
        wxFile file;
        const std::size_t htmlLen = std::strlen(html);
        // NB: wxFile::Write(const void*, size_t) ritorna il numero di byte
        // EFFETTIVAMENTE scritti (size_t), non un bool: un confronto diretto
        // con la lunghezza attesa è l'unico modo per accorgersi di una
        // scrittura parziale (disco pieno a metà, ecc.), che altrimenti la
        // conversione implicita size_t->bool (non zero = true) lascerebbe
        // passare inosservata.
        const bool ok = file.Create(path, true /* sovrascrivi se esiste */)
            && file.Write(html, htmlLen) == htmlLen;
        if (!ok) {
            wxMessageBox(tr(StringId::MfGuideWriteError),
                         kAppName, wxICON_ERROR | wxOK, this);
            return;
        }
        file.Close();
        wxLaunchDefaultBrowser(wxFileName::FileNameToURL(path));
    }, kMenuHelp);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxAboutDialogInfo info;
        info.SetName(kAppName);
        info.SetVersion(kAppVersion);
        info.SetIcon(wxIcon(AliveMonitor_xpm));
        info.SetDescription(tr(StringId::MfAboutDescription));
        info.SetCopyright(wxString::FromUTF8("© 2026 Leonardo Catalano"));
        info.SetLicence(tr(StringId::MfAboutLicence));
        info.SetWebSite("https://github.com/leocata72/AliveMonitor");
        info.AddDeveloper("Leonardo Catalano");
        info.AddDeveloper("Claude (Anthropic)");
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
