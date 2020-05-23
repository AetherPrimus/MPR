// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <cstddef>
#include <string>
#include <limits>
#include <wx/button.h>
#include <wx/spinctrl.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/valnum.h>

#include "DolphinWX/Debugger/MatrixPanel.h"
#include "DolphinWX/WxUtils.h"
#include "Common/StringUtil.h"
#include "VideoCommon/Debugger.h"

#include "Core/ConfigManager.h"
#include "Core/HW/Memmap.h"
#include "Core/PrimeHack/Transform.h"

MatrixPanel::MatrixPanel(wxWindow* parent, wxWindowID id, const wxPoint& position,
                                   const wxSize& size, long style, const wxString& title)
    : wxPanel(parent, id, position, size, style, title)
{
  const int space3 = FromDIP(3);

  m_Write = new wxButton(this, wxID_ANY, _("Write Transform to Address"));
  m_Write->Bind(wxEVT_BUTTON, &MatrixPanel::WriteTransformClicked, this);

  m_Read = new wxButton(this, wxID_ANY, _("Read Transform from Address"));
  m_Read->Bind(wxEVT_BUTTON, &MatrixPanel::ReadTransformClicked, this);

  m_Address = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, *new wxSize(75, -1));
  wxSize countersize = *new wxSize(75, -1);

  constexpr float MIN = std::numeric_limits<float>::min();
  constexpr float MAX = std::numeric_limits<float>::max();

  pit = new wxTextCtrl(this, wxID_ANY, "",
    wxDefaultPosition, countersize, 0, wxDefaultValidator, _("Pitch"));
  yaw = new wxTextCtrl(this, wxID_ANY, "",
    wxDefaultPosition, countersize, 0, wxDefaultValidator, _("Yaw"));
  rol = new wxTextCtrl(this, wxID_ANY, "",
    wxDefaultPosition, countersize, 0, wxDefaultValidator, _("Roll"));

  wxBoxSizer* sMain = new wxBoxSizer(wxVERTICAL);

  wxGridSizer* const grid_szr = new wxGridSizer(0, 2, 5, 5);
  grid_szr->Add(new wxStaticText(this, wxID_ANY, _("Address: ")), 0, wxBOTTOM, space3);
  grid_szr->Add(m_Address);

  grid_szr->Add(new wxStaticText(this, wxID_ANY, _("Pitch: ")), 0, wxBOTTOM, space3);
  grid_szr->Add(pit);

  grid_szr->Add(new wxStaticText(this, wxID_ANY, _("Yaw: ")), 0, wxBOTTOM, space3);
  grid_szr->Add(yaw);

  grid_szr->Add(new wxStaticText(this, wxID_ANY, _("Roll: ")), 0, wxBOTTOM, space3);
  grid_szr->Add(rol);


  wxBoxSizer* const buttons_szr = new wxBoxSizer(wxVERTICAL);
  buttons_szr->AddSpacer(5);
  buttons_szr->Add(m_Write, 0, wxEXPAND);
  buttons_szr->AddSpacer(5);
  buttons_szr->Add(m_Read, 0, wxEXPAND);
  buttons_szr->AddSpacer(5);

  wxStaticBoxSizer* const pFlowCtrlBox = new wxStaticBoxSizer(wxVERTICAL, this, _("Transform"));
  pFlowCtrlBox->Add(grid_szr, 0, wxCENTER, 5);
  pFlowCtrlBox->AddSpacer(15);
  pFlowCtrlBox->Add(buttons_szr, 1, wxEXPAND);

  sMain->Add(pFlowCtrlBox);

  SetSizerAndFit(sMain);
}

void MatrixPanel::WriteTransformClicked(wxCommandEvent& e)
{
  u32 addr;

  if ((addr = TryGetAddr()) == NULL)
    return;

  UpdateXYZ(addr);

  double y;
  double p;
  double r;

  if (yaw->GetValue().IsEmpty())
    y = m_y;
  else {
    if (!yaw->GetValue().ToDouble(&y)) {
      WxUtils::ShowErrorDialog(_("Yaw could not be read."));
      return;
    }
  }

  if (pit->GetValue().IsEmpty())
    p = m_p;
  else {
    if (!pit->GetValue().ToDouble(&p)) {
      WxUtils::ShowErrorDialog(_("Pitch could not be read."));
      return;
    }
  }

  if (rol->GetValue().IsEmpty())
    r = m_r;
  else {
    if (!rol->GetValue().ToDouble(&r)) {
      WxUtils::ShowErrorDialog(_("Roll could not be read."));
      return;
    }
  }

  prime::Transform t;
  t.build_rotation(y, p, r);

  // swapping col 2 and col 3 to match a RetroStudio transform
  float m_temp[3][4];
  m_temp[1][0] = t.m[2][0];
  m_temp[1][1] = t.m[2][1];
  m_temp[1][2] = t.m[2][2];
  m_temp[1][3] = t.m[2][3];

  t.m[2][0] = t.m[1][0];
  t.m[2][1] = t.m[1][1];
  t.m[2][2] = t.m[1][2];
  t.m[2][3] = t.m[1][3];

  t.m[1][0] = m_temp[1][0];
  t.m[1][1] = m_temp[1][1];
  t.m[1][2] = m_temp[1][2];
  t.m[1][3] = m_temp[1][3];

  t.write_to(addr);
}

void MatrixPanel::ReadTransformClicked(wxCommandEvent& e)
{
  u32 addr;

  if ((addr = TryGetAddr()) == NULL)
    return;

  UpdateXYZ(addr);

  pit->SetValue(wxString::Format(wxT("%f"), m_p));
  yaw->SetValue(wxString::Format(wxT("%f"), m_y));
  rol->SetValue(wxString::Format(wxT("%f"), m_r));
}

void MatrixPanel::UpdateXYZ(u32 addr)
{
  prime::Transform t;
  t.read_from(addr);

  m_p = atan2f(prime::vec3(0, 0, 1).dot(t.up()),
    prime::vec3(0, 1, 0).dot(t.up())) * (180 / M_PI);

  m_y = atan2f(prime::vec3(1, 0, 0).dot(t.fwd()),
    prime::vec3(0, 0, 1).dot(t.fwd())) * (180 / M_PI);

  m_r = atan2f(prime::vec3(0, 1, 0).dot(t.right()),
    prime::vec3(1, 0, 0).dot(t.right())) * (180 / M_PI);
}

u32 MatrixPanel::TryGetAddr()
{
  wxString str = m_Address->GetValue().Trim().Trim(false);

  if (!str.StartsWith("0x"))
    str = "0x" + str;

  u32 addr;
  if (!TryParse(WxStrToStr(str), &addr))
  {
    WxUtils::ShowErrorDialog(wxString::Format(_("Invalid address: %s"), str.c_str()));
    return NULL;
  }

  return addr;
}

MatrixPanel::~MatrixPanel()
{

}
