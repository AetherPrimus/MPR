// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "DolphinWX/Input/InputConfigDiag.h"
#include <wx/notebook.h>
#include <wx/radiobut.h>

class WiimoteInputConfigDialog final : public InputConfigDialog
{
public:
  WiimoteInputConfigDialog(wxWindow* parent, InputConfig& config, const wxString& name,
                           int port_num = 0);
  void AddPrimeHackTab(wxNotebook* notebook);
  void OnPageChanged(wxBookCtrlEvent& ev);
  void OnModeChanged(wxCommandEvent& ex);
  void OnUpdateUI(wxEvent& ex);
  void UpdateUI(bool checked);

  ControlGroupBox* m_primehack_stick;
  ControlGroupBox* m_primehack_modes;
};
