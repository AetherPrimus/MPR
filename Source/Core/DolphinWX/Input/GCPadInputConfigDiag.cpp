// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.
#include <wx/radiobut.h>

#include "DolphinWX/Input/GCPadInputConfigDiag.h"

#include <wx/notebook.h>
#include "Core/HW/GCPad.h"
#include "Core/HW/GCPadEmu.h"
#include <Core/ConfigManager.h>
#include <include/wx/msgdlg.h>

GCPadInputConfigDialog::GCPadInputConfigDialog(wxWindow* const parent, InputConfig& config,
                                               const wxString& name, const int port_num)
    : InputConfigDialog(parent, config, name, port_num)
{
  const int space5 = FromDIP(5);

  auto* const device_chooser = CreateDeviceChooserGroupBox();
  auto* const reset_sizer = CreaterResetGroupBox(wxHORIZONTAL);
  auto* const profile_chooser = CreateProfileChooserGroupBox();

  auto* const notebook = new wxNotebook(this, wxID_ANY);

  // General
  auto* const tab_general = new wxPanel(notebook);

  auto* const group_box_buttons =
      new ControlGroupBox(Pad::GetGroup(port_num, PadGroup::Buttons), tab_general, this);
  auto* const group_box_main_stick =
      new ControlGroupBox(Pad::GetGroup(port_num, PadGroup::MainStick), tab_general, this);
  auto* const group_box_c_stick =
      new ControlGroupBox(Pad::GetGroup(port_num, PadGroup::CStick), tab_general, this);
  auto* const group_box_dpad =
      new ControlGroupBox(Pad::GetGroup(port_num, PadGroup::DPad), tab_general, this);
  auto* const group_box_triggers =
      new ControlGroupBox(Pad::GetGroup(port_num, PadGroup::Triggers), tab_general, this);
  auto* const group_box_rumble =
      new ControlGroupBox(Pad::GetGroup(port_num, PadGroup::Rumble), tab_general, this);
  auto* const group_box_options =
      new ControlGroupBox(Pad::GetGroup(port_num, PadGroup::Options), tab_general, this);

  auto* const triggers_rumble_sizer = new wxBoxSizer(wxVERTICAL);
  triggers_rumble_sizer->Add(group_box_triggers, 0, wxEXPAND);
  triggers_rumble_sizer->AddSpacer(space5);
  triggers_rumble_sizer->Add(group_box_rumble, 0, wxEXPAND);

  auto* const dpad_options_sizer = new wxBoxSizer(wxVERTICAL);
  dpad_options_sizer->Add(group_box_dpad, 0, wxEXPAND);
  dpad_options_sizer->AddSpacer(space5);
  dpad_options_sizer->Add(group_box_options, 0, wxEXPAND);

  auto* const controls_sizer = new wxBoxSizer(wxHORIZONTAL);
  controls_sizer->AddSpacer(space5);
  controls_sizer->Add(group_box_buttons, 0, wxEXPAND | wxTOP, space5);
  controls_sizer->AddSpacer(space5);
  controls_sizer->Add(group_box_main_stick, 0, wxEXPAND | wxTOP, space5);
  controls_sizer->AddSpacer(space5);
  controls_sizer->Add(group_box_c_stick, 0, wxEXPAND | wxTOP, space5);
  controls_sizer->AddSpacer(space5);
  controls_sizer->Add(triggers_rumble_sizer, 0, wxEXPAND | wxTOP, space5);
  controls_sizer->AddSpacer(space5);
  controls_sizer->Add(dpad_options_sizer, 0, wxEXPAND | wxTOP, space5);
  controls_sizer->AddSpacer(space5);

  tab_general->SetSizerAndFit(controls_sizer);
  notebook->AddPage(tab_general, _("General"));

  notebook->SetSelection(0);

  auto* const dio = new wxBoxSizer(wxHORIZONTAL);
  dio->AddSpacer(space5);
  dio->Add(device_chooser, 2, wxEXPAND);
  dio->AddSpacer(space5);
  dio->Add(reset_sizer, 1, wxEXPAND);
  dio->AddSpacer(space5);
  dio->Add(profile_chooser, 2, wxEXPAND);
  dio->AddSpacer(space5);

  auto* const szr_main = new wxBoxSizer(wxVERTICAL);
  szr_main->AddSpacer(space5);
  szr_main->Add(dio);
  szr_main->AddSpacer(space5);
  szr_main->Add(notebook, 1, wxEXPAND | wxLEFT | wxRIGHT, space5);
  szr_main->AddSpacer(space5);
  szr_main->Add(CreateButtonSizer(wxCLOSE | wxNO_DEFAULT), 0, wxEXPAND | wxLEFT | wxRIGHT, space5);
  szr_main->AddSpacer(space5);

  // PrimeHack Tab
  AddPrimeHackTab(notebook);

  SetSizerAndFit(szr_main);
  Center();

  UpdateDeviceComboBox();
  UpdateProfileComboBox();

  Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &GCPadInputConfigDialog::OnPageChanged, this);
  Bind(wxEVT_RADIOBUTTON, &GCPadInputConfigDialog::OnModeChanged, this);
  Bind(wxEVT_UPDATE_UI, &GCPadInputConfigDialog::OnUpdateUI, this);

  UpdateGUI();
}

void GCPadInputConfigDialog::AddPrimeHackTab(wxNotebook* notebook)
{
  const int space5 = FromDIP(5);
  const int space3 = FromDIP(3);

  auto* const tab_primehack = new wxPanel(notebook);

  auto* const m_primehack_camera =
    new ControlGroupBox(Pad::GetGroup(0, PadGroup::PrimeCameraOpt), tab_primehack, this);

  //auto* const m_primehack_misc =
  //  new ControlGroupBox(Pad::GetGroup(0, PadGroup::PrimeMisc), tab_primehack, this); Misc doesn't contain anything GCN uses currently.

  m_primehack_modes =
    new ControlGroupBox(Pad::GetGroup(0, PadGroup::PrimeControlMode), tab_primehack, this);

  m_primehack_stick =
    new ControlGroupBox(Pad::GetGroup(0, PadGroup::PrimeCameraControl), tab_primehack, this);

  auto* const camera_sizer = new wxBoxSizer(wxVERTICAL);
  camera_sizer->Add(m_primehack_camera, 0, wxEXPAND | wxTOP, space5);

  auto* const general_sizer = new wxBoxSizer(wxHORIZONTAL);

  auto* const sub_sizer1 = new wxBoxSizer(wxVERTICAL);
  auto* const sub_sizer2 = new wxBoxSizer(wxVERTICAL);

  sub_sizer1->AddSpacer(space5);
  sub_sizer1->Add(m_primehack_modes, 0, wxEXPAND | wxRIGHT, space5);
  sub_sizer1->AddSpacer(space5);
  //sub_sizer1->Add(m_primehack_misc, 0, wxEXPAND | wxRIGHT, space5); Ready to be added when Misc is used by GCN.
  //sub_sizer1->AddSpacer(space5);
  sub_sizer1->Add(camera_sizer, 0, wxEXPAND | wxRIGHT, space5);
  sub_sizer1->AddSpacer(space5);

  general_sizer->AddSpacer(5);
  general_sizer->Add(sub_sizer1);
  general_sizer->AddSpacer(5);

  sub_sizer2->AddSpacer(space5);
  sub_sizer2->Add(m_primehack_stick, 0, wxEXPAND | wxRIGHT, space5);
  sub_sizer2->AddSpacer(space5);

  general_sizer->Add(sub_sizer2);

  tab_primehack->SetSizerAndFit(general_sizer);

  notebook->AddPage(tab_primehack, _("PrimeHack"));

  UpdateUI(Pad::PrimeUseController());
}


void GCPadInputConfigDialog::OnPageChanged(wxBookCtrlEvent& ev)
{
  if (!SConfig::GetInstance().bEnablePrimeHack)
    if (ev.GetSelection() == 1)
      wxMessageBox("PrimeHack has not been enabled. None of the controls or settings in the PrimeHack tab will work until it is enabled in the Config window.", "PrimeHack Settings");
}

void GCPadInputConfigDialog::OnUpdateUI(wxEvent& ev)
{
  UpdateUI(Pad::PrimeUseController());
}

void GCPadInputConfigDialog::UpdateUI(bool checked)
{
  if (m_primehack_stick->control_buttons[0]->IsEnabled() != checked) {
    for (ControlButton* cb : m_primehack_stick->control_buttons)
      cb->Enable(checked);

    for (PadSetting* cb : m_primehack_stick->options)
      cb->wxcontrol->Enable(checked);
  } 
}

void GCPadInputConfigDialog::OnModeChanged(wxCommandEvent& ex)
{
  wxRadioButton* const btn = (wxRadioButton*)ex.GetEventObject();
  bool checked = btn->GetValue();

  if (btn->GetLabel().StartsWith("M"))
    checked = !checked;

  Pad::PrimeSetMode(checked);
  UpdateUI(checked);
}
