// Copyright 2010 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "Core/HW/GCPadEmu.h"

#include "Common/Common.h"
#include "Common/CommonTypes.h"

#include "InputCommon/ControllerEmu/Control/Input.h"
#include "InputCommon/ControllerEmu/Control/Output.h"
#include "InputCommon/ControllerEmu/ControlGroup/AnalogStick.h"
#include "InputCommon/ControllerEmu/ControlGroup/Buttons.h"
#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"
#include "InputCommon/ControllerEmu/ControlGroup/MixedTriggers.h"
#include "InputCommon/ControllerEmu/Setting/BooleanSetting.h"
#include "InputCommon/GCPadStatus.h"

static const u16 button_bitmasks[] = {
  PAD_BUTTON_A,
  PAD_BUTTON_B,
  PAD_BUTTON_X,
  PAD_BUTTON_Y,
  PAD_TRIGGER_Z,
  PAD_BUTTON_START,
  0  // MIC HAX
};

static const u16 trigger_bitmasks[] = {
  PAD_TRIGGER_L, PAD_TRIGGER_R,
};

static const u16 dpad_bitmasks[] = { PAD_BUTTON_UP, PAD_BUTTON_DOWN, PAD_BUTTON_LEFT,
PAD_BUTTON_RIGHT };

static const char* const named_buttons[] = { "A", "B", "X", "Y", "Z", "Start" };

static const char* const named_triggers[] = {
  // i18n: The left trigger button (labeled L on real controllers)
  _trans("L"),
  // i18n: The right trigger button (labeled R on real controllers)
  _trans("R"),
  // i18n: The left trigger button (labeled L on real controllers) used as an analog input
  _trans("L-Analog"),
  // i18n: The right trigger button (labeled R on real controllers) used as an analog input
  _trans("R-Analog") };

GCPad::GCPad(const unsigned int index) : m_index(index)
{
  // buttons
  groups.emplace_back(m_buttons = new ControllerEmu::Buttons(_trans("Buttons")));
  for (const char* named_button : named_buttons)
  {
    const std::string& ui_name =
      // i18n: The START/PAUSE button on GameCube controllers
      (named_button == std::string("Start")) ? _trans("START") : named_button;
    m_buttons->controls.emplace_back(new ControllerEmu::Input(named_button, ui_name));
  }

  // sticks
  groups.emplace_back(m_main_stick = new ControllerEmu::AnalogStick(
    "Main Stick", _trans("Control Stick"), DEFAULT_PAD_STICK_RADIUS));
  groups.emplace_back(m_c_stick = new ControllerEmu::AnalogStick("C-Stick", _trans("C Stick"),
    DEFAULT_PAD_STICK_RADIUS));

  // triggers
  groups.emplace_back(m_triggers = new ControllerEmu::MixedTriggers(_trans("Triggers")));
  for (const char* named_trigger : named_triggers)
    m_triggers->controls.emplace_back(new ControllerEmu::Input(named_trigger));

  // rumble
  groups.emplace_back(m_rumble = new ControllerEmu::ControlGroup(_trans("Rumble")));
  m_rumble->controls.emplace_back(new ControllerEmu::Output(_trans("Motor")));

  // Microphone
  groups.emplace_back(m_mic = new ControllerEmu::Buttons(_trans("Microphone")));
  m_mic->controls.emplace_back(new ControllerEmu::Input(_trans("Button")));

  // dpad
  groups.emplace_back(m_dpad = new ControllerEmu::Buttons(_trans("D-Pad")));
  for (const char* named_direction : named_directions)
    m_dpad->controls.emplace_back(new ControllerEmu::Input(named_direction));

  // options
  groups.emplace_back(m_options = new ControllerEmu::ControlGroup(_trans("Options")));
  m_options->boolean_settings.emplace_back(
    // i18n: Treat a controller as always being connected regardless of what
    // devices the user actually has plugged in
    m_always_connected = new ControllerEmu::BooleanSetting(_trans("Always Connected"), false));
  m_options->boolean_settings.emplace_back(std::make_unique<ControllerEmu::BooleanSetting>(
    _trans("Iterative Input"), false, ControllerEmu::SettingType::VIRTUAL));

  groups.emplace_back(m_primehack_camera =
    new ControllerEmu::ControlGroup(_trans("PrimeHack"), "Camera"));

  m_primehack_camera->numeric_settings.emplace_back(
    m_primehack_camera_sensitivity =
    new ControllerEmu::NumericSetting(_trans("Camera Sensitivity"), 0.15, 1, 200));

  m_primehack_camera->boolean_settings.emplace_back(
    m_primehack_invert_x = new ControllerEmu::BooleanSetting("Invert X Axis", false));

  m_primehack_camera->boolean_settings.emplace_back(
    m_primehack_invert_y = new ControllerEmu::BooleanSetting("Invert Y Axis", false));

  groups.emplace_back(m_primehack_stick = new ControllerEmu::AnalogStick(_trans("Camera Control"), 1.0f));

  m_primehack_stick->numeric_settings.emplace_back(
    m_primehack_horizontal_sensitivity = new ControllerEmu::NumericSetting(_trans("Horizontal Sensitivity"), 0.45, 1, 100));


  m_primehack_stick->numeric_settings.emplace_back(
    m_primehack_vertical_sensitivity = new ControllerEmu::NumericSetting(_trans("Vertical Sensitivity"), 0.35, 1, 100));

  groups.emplace_back(m_primehack_misc =
    new ControllerEmu::ControlGroup(_trans("Miscellaneous")));

  groups.emplace_back(m_primehack_modes =
    new ControllerEmu::PrimeHackModes(_trans("Control Mode")));
}

std::string GCPad::GetName() const
{
  return std::string("GCPad") + char('1' + m_index);
}

ControllerEmu::ControlGroup* GCPad::GetGroup(PadGroup group)
{
  switch (group)
  {
  case PadGroup::Buttons:
    return m_buttons;
  case PadGroup::MainStick:
    return m_main_stick;
  case PadGroup::CStick:
    return m_c_stick;
  case PadGroup::DPad:
    return m_dpad;
  case PadGroup::Triggers:
    return m_triggers;
  case PadGroup::Rumble:
    return m_rumble;
  case PadGroup::Mic:
    return m_mic;
  case PadGroup::Options:
    return m_options;
  case PadGroup::PrimeCameraControl:
    return m_primehack_stick;
  case PadGroup::PrimeCameraOpt:
    return m_primehack_camera;
  case PadGroup::PrimeControlMode:
    return m_primehack_modes;
  case PadGroup::PrimeMisc:
    return m_primehack_misc;
  default:
    return nullptr;
  }
}

GCPadStatus GCPad::GetInput() const
{
  const auto lock = GetStateLock();

  ControlState x, y, triggers[2];
  GCPadStatus pad = {};

  if (!(m_always_connected->GetValue() || IsDefaultDeviceConnected()))
  {
    pad.isConnected = false;
    return pad;
  }

  // buttons
  m_buttons->GetState(&pad.button, button_bitmasks);

  // set analog A/B analog to full or w/e, prolly not needed
  if (pad.button & PAD_BUTTON_A)
    pad.analogA = 0xFF;
  if (pad.button & PAD_BUTTON_B)
    pad.analogB = 0xFF;

  // dpad
  m_dpad->GetState(&pad.button, dpad_bitmasks);

  // sticks
  m_main_stick->GetState(&x, &y);
  pad.stickX =
    static_cast<u8>(GCPadStatus::MAIN_STICK_CENTER_X + (x * GCPadStatus::MAIN_STICK_RADIUS));
  pad.stickY =
    static_cast<u8>(GCPadStatus::MAIN_STICK_CENTER_Y + (y * GCPadStatus::MAIN_STICK_RADIUS));

  m_c_stick->GetState(&x, &y);
  pad.substickX =
    static_cast<u8>(GCPadStatus::C_STICK_CENTER_X + (x * GCPadStatus::C_STICK_RADIUS));
  pad.substickY =
    static_cast<u8>(GCPadStatus::C_STICK_CENTER_Y + (y * GCPadStatus::C_STICK_RADIUS));

  // triggers
  m_triggers->GetState(&pad.button, trigger_bitmasks, triggers);
  pad.triggerLeft = static_cast<u8>(triggers[0] * 0xFF);
  pad.triggerRight = static_cast<u8>(triggers[1] * 0xFF);

  return pad;
}

void GCPad::SetOutput(const ControlState strength)
{
  const auto lock = GetStateLock();
  m_rumble->controls[0]->control_ref->State(strength);
}

void GCPad::LoadDefaults(const ControllerInterface& ciface)
{
  EmulatedController::LoadDefaults(ciface);

  // Buttons
  m_buttons->SetControlExpression(0, "`Click 0`");  // A
  m_buttons->SetControlExpression(1, "SPACE");  // B
  m_buttons->SetControlExpression(2, "Ctrl");  // X
  m_buttons->SetControlExpression(3, "F");  // Y
  m_buttons->SetControlExpression(4, "TAB");  // Z
#ifdef _WIN32
  m_buttons->SetControlExpression(5, "GRAVE");  // Start
#else
                                            // OS X/Linux
  m_buttons->SetControlExpression(5, "GRAVE");  // Start
#endif

                                                            // stick modifiers to 50 %
  m_main_stick->controls[4]->control_ref->range = 0.5f;
  m_c_stick->controls[4]->control_ref->range = 0.5f;

  // D-Pad
  m_dpad->SetControlExpression(0, "E & `1`");  // Up
  m_dpad->SetControlExpression(1, "E & `3`");  // Down
  m_dpad->SetControlExpression(2, "E & `2`");  // Left
  m_dpad->SetControlExpression(3, "E & `4`");  // Right

  // C Stick
  m_c_stick->SetControlExpression(0, "!E & `1`");  // Up
  m_c_stick->SetControlExpression(1, "!E & `3`");  // Down
  m_c_stick->SetControlExpression(2, "!E & `4`");  // Left
  m_c_stick->SetControlExpression(3, "!E & `2`");  // Right

                                                   // Control Stick
  m_main_stick->SetControlExpression(0, "W | UP");      // Up
  m_main_stick->SetControlExpression(1, "S | DOWN");    // Down
  m_main_stick->SetControlExpression(2, "A | LEFT");    // Left
  m_main_stick->SetControlExpression(3, "D | RIGHT");   // Right

                                                     // Triggers
  m_triggers->SetControlExpression(0, "LSHIFT");  // L
  m_triggers->SetControlExpression(2, "LSHIFT");  // R
}

bool GCPad::GetMicButton() const
{
  const auto lock = GetStateLock();
  return (0.0f != m_mic->controls.back()->control_ref->State());
}

bool GCPad::CheckSpringBallCtrl()
{
  return m_primehack_misc->controls[0].get()->control_ref->State() > 0.5;
}

std::tuple<double, double> GCPad::GetPrimeStickXY()
{
  double x,y;
  m_primehack_stick->GetState(&x, &y);

  return std::make_tuple(x * m_primehack_horizontal_sensitivity->GetValue() * 100, y * m_primehack_vertical_sensitivity->GetValue() * -100);
}

bool GCPad::PrimeControllerMode()
{
  return m_primehack_modes->GetSelectedDevice() == 1;
}

void GCPad::SetPrimeMode(bool controller)
{
  m_primehack_modes->SetSelectedDevice(controller ? 1 : 0);
}

std::tuple<double, double, bool, bool> GCPad::GetPrimeSettings()
{
  std::tuple t = std::make_tuple(
    m_primehack_camera_sensitivity->GetValue() * 100, 0.f, m_primehack_invert_x->GetValue(),
    m_primehack_invert_y->GetValue());

  return t;
}
