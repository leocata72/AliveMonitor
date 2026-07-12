/**
 * @file ToolbarPanel.cpp
 * @brief Implementazione della barra superiore.
 */
#include "view/ToolbarPanel.h"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "i18n/Strings.h"
#include "view/LedIndicator.h"

namespace am {

ToolbarPanel::ToolbarPanel(wxWindow* parent, IUserActions& actions)
    : wxPanel(parent)
    , actions_(actions)
{
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    // LED verde (connesso) / rosso (non connesso).
    led_ = new LedIndicator(this, wxColour(46, 204, 64), wxColour(220, 53, 53));
    sizer->Add(led_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);

    statusText_ = new wxStaticText(this, wxID_ANY, tr(StringId::TbSearching));
    {
        wxFont bold = statusText_->GetFont();
        bold.MakeBold();
        statusText_->SetFont(bold);
    }
    sizer->Add(statusText_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    portText_ = new wxStaticText(this, wxID_ANY, tr(StringId::TbPortPrefix) + "-");
    sizer->Add(portText_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    // "Baud" è un termine tecnico universale (non tradotto, come Hz o CRC).
    baudText_ = new wxStaticText(this, wxID_ANY, "Baud: -");
    sizer->Add(baudText_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    firmwareText_ = new wxStaticText(this, wxID_ANY, tr(StringId::TbFwPrefix) + "-");
    sizer->Add(firmwareText_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    messageText_ = new wxStaticText(this, wxID_ANY, wxString());
    messageText_->SetForegroundColour(wxColour(150, 90, 0));
    sizer->Add(messageText_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    connectButton_ = new wxButton(this, wxID_ANY, tr(StringId::TbConnectBtn));
    connectButton_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        actions_.onConnectRequested();
    });
    sizer->Add(connectButton_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    disconnectButton_ = new wxButton(this, wxID_ANY, tr(StringId::TbDisconnectBtn));
    disconnectButton_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        actions_.onDisconnectRequested();
    });
    sizer->Add(disconnectButton_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    SetSizer(sizer);
}

void ToolbarPanel::updateFrom(const BoardState::Snapshot& snapshot,
                              const wxString& message)
{
    switch (snapshot.connection) {
    case ConnectionState::Connected:
        led_->setOn(true);
        statusText_->SetLabel(tr(StringId::TbConnected));
        break;
    case ConnectionState::Scanning:
        led_->setOn(false);
        statusText_->SetLabel(tr(StringId::TbSearching));
        break;
    case ConnectionState::Disconnected:
        led_->setOn(false);
        statusText_->SetLabel(tr(StringId::TbDisconnected));
        break;
    }

    portText_->SetLabel(tr(StringId::TbPortPrefix)
        + (snapshot.portName.empty() ? wxString("-") : wxString(snapshot.portName)));
    baudText_->SetLabel(wxString::Format("Baud: %u", snapshot.baudRate));
    firmwareText_->SetLabel(tr(StringId::TbFwPrefix)
        + (snapshot.firmwareVersion.empty() ? wxString("-")
                                            : wxString(snapshot.firmwareVersion)));
    if (messageText_->GetLabel() != message) {
        messageText_->SetLabel(message);
    }

    connectButton_->Enable(snapshot.connection == ConnectionState::Disconnected);
    disconnectButton_->Enable(snapshot.connection != ConnectionState::Disconnected);

    Layout();
}

} // namespace am
