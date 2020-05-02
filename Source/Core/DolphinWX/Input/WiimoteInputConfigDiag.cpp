// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <wx/notebook.h>
#include <wx/radiobut.h>
#include <wx/stattext.h>

#include "DolphinWX/Input/WiimoteInputConfigDiag.h"

#include "Core/HW/Wiimote.h"
#include "Core/HW/WiimoteEmu/WiimoteEmu.h"

#include "InputCommon/ControllerEmu/Setting/BooleanSetting.h"
#include "InputCommon/ControllerEmu/Setting/NumericSetting.h"
#include <Core/ConfigManager.h>
#include <include/wx/msgdlg.h>

WiimoteInputConfigDialog::WiimoteInputConfigDialog(wxWindow* const parent, InputConfig& config,
                                                   const wxString& name, const int port_num)
    : InputConfigDialog(parent, config, name, port_num)
{
  const int space5 = FromDIP(5);

  auto* const device_chooser = CreateDeviceChooserGroupBox();
  auto* const reset_sizer = CreaterResetGroupBox(wxVERTICAL);
  auto* const profile_chooser = CreateProfileChooserGroupBox();

  auto* const notebook = new wxNotebook(this, wxID_ANY);

  // General and Options
  auto* const tab_general = new wxPanel(notebook);

  auto* const group_box_buttons = new ControlGroupBox(
      Wiimote::GetWiimoteGroup(port_num, WiimoteEmu::WiimoteGroup::Buttons), tab_general, this);
  auto* const group_box_dpad = new ControlGroupBox(
      Wiimote::GetWiimoteGroup(port_num, WiimoteEmu::WiimoteGroup::DPad), tab_general, this);
  auto* const group_box_rumble = new ControlGroupBox(
      Wiimote::GetWiimoteGroup(port_num, WiimoteEmu::WiimoteGroup::Rumble), tab_general, this);
  auto* const group_box_extension = new ControlGroupBox(
      Wiimote::GetWiimoteGroup(port_num, WiimoteEmu::WiimoteGroup::Extension), tab_general, this);
  auto* const group_box_options = new ControlGroupBox(
      Wiimote::GetWiimoteGroup(port_num, WiimoteEmu::WiimoteGroup::Options), tab_general, this);
  auto* const group_box_hotkeys = new ControlGroupBox(
      Wiimote::GetWiimoteGroup(port_num, WiimoteEmu::WiimoteGroup::Hotkeys), tab_general, this);

  auto* const dpad_extension_rumble_sizer = new wxBoxSizer(wxVERTICAL);
  dpad_extension_rumble_sizer->Add(group_box_dpad, 0, wxEXPAND);
  dpad_extension_rumble_sizer->AddSpacer(space5);
  dpad_extension_rumble_sizer->Add(group_box_extension, 0, wxEXPAND);
  dpad_extension_rumble_sizer->AddSpacer(space5);
  dpad_extension_rumble_sizer->Add(group_box_rumble, 0, wxEXPAND);

  auto* const options_hotkeys_sizer = new wxBoxSizer(wxVERTICAL);
  options_hotkeys_sizer->Add(group_box_options, 0, wxEXPAND);
  options_hotkeys_sizer->AddSpacer(space5);
  options_hotkeys_sizer->Add(group_box_hotkeys, 0, wxEXPAND);

  auto* const general_options_sizer = new wxBoxSizer(wxHORIZONTAL);
  general_options_sizer->AddSpacer(space5);
  general_options_sizer->Add(group_box_buttons, 0, wxEXPAND | wxTOP, space5);
  general_options_sizer->AddSpacer(space5);
  general_options_sizer->Add(dpad_extension_rumble_sizer, 0, wxEXPAND | wxTOP, space5);
  general_options_sizer->AddSpacer(space5);
  general_options_sizer->Add(options_hotkeys_sizer, 0, wxEXPAND | wxTOP, space5);
  general_options_sizer->AddSpacer(space5);

  tab_general->SetSizerAndFit(general_options_sizer);

  notebook->AddPage(tab_general, _("General and Options"));

  // Motion Controls and IR
  auto* const tab_motion_controls_ir = new wxPanel(notebook);

  auto* const group_box_shake =
      new ControlGroupBox(Wiimote::GetWiimoteGroup(port_num, WiimoteEmu::WiimoteGroup::Shake),
                          tab_motion_controls_ir, this);
  auto* const group_box_ir =
      new ControlGroupBox(Wiimote::GetWiimoteGroup(port_num, WiimoteEmu::WiimoteGroup::IR),
                          tab_motion_controls_ir, this);
  auto* const group_box_tilt =
      new ControlGroupBox(Wiimote::GetWiimoteGroup(port_num, WiimoteEmu::WiimoteGroup::Tilt),
                          tab_motion_controls_ir, this);
  auto* const group_box_swing =
      new ControlGroupBox(Wiimote::GetWiimoteGroup(port_num, WiimoteEmu::WiimoteGroup::Swing),
                          tab_motion_controls_ir, this);

  auto* const swing_shake_sizer = new wxBoxSizer(wxVERTICAL);
  swing_shake_sizer->Add(group_box_swing, 0, wxEXPAND);
  swing_shake_sizer->AddSpacer(space5);
  swing_shake_sizer->Add(group_box_shake, 0, wxEXPAND);

  auto* const motion_controls_ir_sizer = new wxBoxSizer(wxHORIZONTAL);
  motion_controls_ir_sizer->AddSpacer(space5);
  motion_controls_ir_sizer->Add(group_box_ir, 0, wxEXPAND | wxTOP, space5);
  motion_controls_ir_sizer->AddSpacer(space5);
  motion_controls_ir_sizer->Add(group_box_tilt, 0, wxEXPAND | wxTOP, space5);
  motion_controls_ir_sizer->AddSpacer(space5);
  motion_controls_ir_sizer->Add(swing_shake_sizer, 0, wxEXPAND | wxTOP, space5);
  motion_controls_ir_sizer->AddSpacer(space5);

  tab_motion_controls_ir->SetSizerAndFit(motion_controls_ir_sizer);

  // i18n: IR stands for infrared and refers to the pointer functionality of Wii Remotes
  notebook->AddPage(tab_motion_controls_ir, _("Motion Controls and IR"));

  notebook->SetSelection(0);

  auto* const device_profile_sizer = new wxBoxSizer(wxVERTICAL);
  device_profile_sizer->Add(device_chooser, 1, wxEXPAND);
  device_profile_sizer->Add(profile_chooser, 1, wxEXPAND);

  auto* const dio = new wxBoxSizer(wxHORIZONTAL);
  dio->AddSpacer(space5);
  dio->Add(device_profile_sizer, 5, wxEXPAND);
  dio->AddSpacer(space5);
  dio->Add(reset_sizer, 1, wxEXPAND);
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

  Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &WiimoteInputConfigDialog::OnPageChanged, this);
  Bind(wxEVT_RADIOBUTTON, &WiimoteInputConfigDialog::OnModeChanged, this);
  Bind(wxEVT_UPDATE_UI, &WiimoteInputConfigDialog::OnUpdateUI, this);
  //Bind(wx, &WiimoteInputConfigDialog::OnButtonPress, this);

  UpdateGUI();
}

void WiimoteInputConfigDialog::OnPageChanged(wxBookCtrlEvent& ev)
{
  if (!SConfig::GetInstance().bEnablePrimeHack)
    if (ev.GetSelection() == 2)
      wxMessageBox("PrimeHack has not been enabled. None of the controls or settings in the PrimeHack tab will work until it is enabled in the Config window.", "PrimeHack Settings");
}

void WiimoteInputConfigDialog::OnUpdateUI(wxEvent& ev)
{
  UpdateUI(Wiimote::PrimeUseController());
}

void WiimoteInputConfigDialog::UpdateUI(bool checked)
{
  if (m_primehack_stick->control_buttons[0]->IsEnabled() != checked) {
    for (ControlButton* cb : m_primehack_stick->control_buttons)
      cb->Enable(checked);

    for (PadSetting* cb : m_primehack_stick->options)
      cb->wxcontrol->Enable(checked);
  } 
}

void WiimoteInputConfigDialog::OnButtonPress(wxCommandEvent& ev)
{
  bool checked = Wiimote::PrimeUseController();
  m_primehack_modes->mouse_but->SetValue(!checked);
  m_primehack_modes->controller_but->SetValue(checked);
}

void WiimoteInputConfigDialog::OnModeChanged(wxCommandEvent& ex)
{
  wxRadioButton* const btn = (wxRadioButton*)ex.GetEventObject();
  bool checked = btn->GetValue();

  if (btn->GetLabel().StartsWith("M"))
    checked = !checked;

  Wiimote::PrimeSetMode(checked);
  UpdateUI(checked);
}

void WiimoteInputConfigDialog::AddPrimeHackTab(wxNotebook* notebook)
{
  const int space5 = FromDIP(5);
  const int space3 = FromDIP(3);

  auto* const tab_primehack = new wxPanel(notebook);

  auto* const m_primehack_beams = new ControlGroupBox(
    Wiimote::GetWiimoteGroup(0, WiimoteEmu::WiimoteGroup::Beams), tab_primehack, this);

  auto* const m_primehack_visors = new ControlGroupBox(
      Wiimote::GetWiimoteGroup(0, WiimoteEmu::WiimoteGroup::Visors), tab_primehack, this);

  auto* const m_primehack_camera = new ControlGroupBox(
      Wiimote::GetWiimoteGroup(0, WiimoteEmu::WiimoteGroup::Camera), tab_primehack, this);

  auto* const m_primehack_misc = new ControlGroupBox(
    Wiimote::GetWiimoteGroup(0, WiimoteEmu::WiimoteGroup::Misc), tab_primehack, this);

  m_primehack_modes = new ControlGroupBox(
    Wiimote::GetWiimoteGroup(0, WiimoteEmu::WiimoteGroup::Modes), tab_primehack, this);

  m_primehack_stick = new ControlGroupBox(
    Wiimote::GetWiimoteGroup(0, WiimoteEmu::WiimoteGroup::ControlStick), tab_primehack, this);

  auto* const camera_sizer = new wxBoxSizer(wxVERTICAL);
  camera_sizer->Add(m_primehack_camera, 0, wxEXPAND | wxTOP, space5);

  auto* const general_sizer = new wxBoxSizer(wxHORIZONTAL);

  auto* const sub_sizer1 = new wxBoxSizer(wxVERTICAL);
  auto* const sub_sizer2 = new wxBoxSizer(wxVERTICAL);

  sub_sizer1->AddSpacer(space5);
  sub_sizer1->Add(m_primehack_modes, 0, wxEXPAND | wxRIGHT, space5);
  sub_sizer1->AddSpacer(space5);
  sub_sizer1->Add(m_primehack_misc, 0, wxEXPAND | wxRIGHT, space5);
  sub_sizer1->AddSpacer(space5);
  sub_sizer1->Add(m_primehack_beams, 0, wxEXPAND | wxTOP, space5);
  sub_sizer1->AddSpacer(space5);
  sub_sizer1->Add(m_primehack_visors, 0, wxEXPAND | wxTOP, space5);
  sub_sizer1->AddSpacer(space5);

  general_sizer->AddSpacer(5);
  general_sizer->Add(sub_sizer1);
  general_sizer->AddSpacer(5);

  sub_sizer2->AddSpacer(space5);
  sub_sizer2->Add(camera_sizer, 0, wxEXPAND | wxRIGHT, space5);
  sub_sizer2->AddSpacer(space5);
  sub_sizer2->Add(m_primehack_stick, 0, wxEXPAND | wxRIGHT, space5);
  sub_sizer2->AddSpacer(space5);

  general_sizer->Add(sub_sizer2);

  tab_primehack->SetSizerAndFit(general_sizer);

  notebook->AddPage(tab_primehack, _("PrimeHack"));
}
