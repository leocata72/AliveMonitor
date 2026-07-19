/**
 * @file MainFrame.cpp
 * @brief Implementazione della finestra principale.
 */
#include "view/MainFrame.h"

#include <algorithm>
#include <cstring>

#include <wx/aboutdlg.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/gdicmn.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include "Version.h"
#include "i18n/Strings.h"
#include "view/AcquisitionPanel.h"
#include "view/CalibrationPanel.h"
#include "view/DigitalOutputPanel.h"
#include "view/GraphPanel.h"
#include "view/OptionsPanel.h"
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
    //  | Calibration    |                                 |
    //  | [funz. future] |                                 |
    //  +----------------+---------------------------------+
    //  | wxStatusBar nativa (v1.2)                        |
    //  +--------------------------------------------------+
    //
    // Pannello di SFONDO unico figlio del frame, con tutti i controlli come
    // suoi figli: è il pattern canonico wxWidgets. Mettere i controlli
    // direttamente sul wxFrame (com'era fino alla 1.1) ha due difetti
    // concreti su Windows: lo sfondo del frame non ha il colore nativo dei
    // dialoghi (grigio più scuro, si notava fra un pannello e l'altro) e la
    // navigazione da tastiera con TAB fra i controlli non funziona (il
    // giro-TAB è implementato da wxPanel, non dal frame).
    auto* background = new wxPanel(this);
    auto* root = new wxBoxSizer(wxVERTICAL);

    toolbar_ = new ToolbarPanel(background, actions_);
    root->Add(toolbar_, 0, wxEXPAND | wxALL, 4);

    auto* middle = new wxBoxSizer(wxHORIZONTAL);

    auto* left = new wxBoxSizer(wxVERTICAL);
    digital_ = new DigitalOutputPanel(background, actions_);
    acquisition_ = new AcquisitionPanel(background, actions_);
    calibration_ = new CalibrationPanel(background, actions_, calibrations);
    left->Add(digital_, 0, wxEXPAND);
    left->Add(acquisition_, 0, wxEXPAND);
    left->Add(calibration_, 0, wxEXPAND);

    // Riquadro "Opzioni" (ex segnaposto "Funzionalità future"): opzioni di
    // comportamento sotto i pulsanti Salva/Carica della calibrazione.
    options_ = new OptionsPanel(background, actions_, buffer, calibrations);
    left->Add(options_, 0, wxEXPAND);

    left->AddStretchSpacer(1);
    middle->Add(left, 0, wxEXPAND | wxLEFT, 4);

    graph_ = new GraphPanel(background, actions_, buffer, calibrations);
    middle->Add(graph_, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);

    root->Add(middle, 1, wxEXPAND);
    background->SetSizer(root);

    // Il frame contiene SOLO il pannello di sfondo, che riempie l'area client.
    auto* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(background, 1, wxEXPAND);
    SetSizer(frameSizer);

    // Status bar NATIVA (v1.2): gestita dal frame (fuori dal pannello di
    // sfondo, come ogni wxStatusBar); StatusPanel è solo l'adapter che ne
    // aggiorna i campi.
    status_ = std::make_unique<StatusPanel>(CreateStatusBar(StatusPanel::kFieldCount));

    // Dimensione iniziale: abbastanza alta da mostrare per intero la colonna
    // sinistra — pulsanti Salva/Carica della calibrazione inclusi — senza
    // però eccedere l'area utile dello schermo (su monitor piccoli si
    // ripiega sul massimo disponibile: il resto resta raggiungibile
    // ridimensionando). ComputeFittingWindowSize tiene già conto di menu,
    // status bar e bordi; la dimensione minima del pannello di sfondo arriva
    // dal suo sizer, quindi il calcolo sul sizer del frame la include.
    const wxSize fit = frameSizer->ComputeFittingWindowSize(this);
    const wxRect area = wxGetClientDisplayRect();
    SetSize(wxSize(std::min(std::max(1200, fit.x), area.width),
                   std::min(std::max(760, fit.y), area.height)));

    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::onClose, this);
}

MainFrame::~MainFrame() = default;

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
