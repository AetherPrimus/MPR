// Copyright 2009 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <wx/panel.h>
#include "VideoCommon/Debugger.h"
#include "Core/PrimeHack/Transform.h"

class wxButton;
class wxSpinCtrlDouble;
class vec3;

class MatrixPanel : public wxPanel
{
public:
  MatrixPanel(wxWindow* parent, wxWindowID id = wxID_ANY,
                   const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                   long style = wxTAB_TRAVERSAL, const wxString& title = _("Matrix Panel"));

  virtual ~MatrixPanel();

private:
  wxButton* m_Read;
  wxButton* m_Write;
  wxTextCtrl* m_Address;

  wxTextCtrl* pit;
  wxTextCtrl* yaw;
  wxTextCtrl* rol;

  float m_p;
  float m_y;
  float m_r;

  void WriteTransformClicked(wxCommandEvent& e);
  void ReadTransformClicked(wxCommandEvent& e);

  u32 TryGetAddr();
  void UpdateXYZ(u32 addr);
};
