// Copyright 2010 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <string>

#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"
#include "InputCommon/ControllerEmu/ControllerEmu.h"
#include "InputCommon/ControllerEmu/ControlGroup/PrimeHackModes.h"

struct GCPadStatus;

namespace ControllerEmu
{
class AnalogStick;
class Buttons;
class MixedTriggers;
class PrimeHackModes;
}

enum class PadGroup
{
  Buttons,
  MainStick,
  CStick,
  DPad,
  Triggers,
  Rumble,
  Mic,
  Options,
  PrimeControlMode,
  PrimeMisc,
  PrimeCameraOpt,
  PrimeCameraControl,
};

class GCPad : public ControllerEmu::EmulatedController
{
public:
  explicit GCPad(unsigned int index);
  GCPadStatus GetInput() const;
  void SetOutput(const ControlState strength);

  bool GetMicButton() const;

  std::string GetName() const override;

  ControllerEmu::ControlGroup* GetGroup(PadGroup group);

  void LoadDefaults(const ControllerInterface& ciface) override;

  bool CheckSpringBallCtrl();
  bool PrimeControllerMode();

  void SetPrimeMode(bool controller);

  std::tuple<double, double> GetPrimeStickXY();

  std::tuple<double, double, double, bool, bool> GetPrimeSettings();

private:
  ControllerEmu::Buttons* m_buttons;
  ControllerEmu::AnalogStick* m_main_stick;
  ControllerEmu::AnalogStick* m_c_stick;
  ControllerEmu::Buttons* m_dpad;
  ControllerEmu::MixedTriggers* m_triggers;
  ControllerEmu::ControlGroup* m_rumble;
  ControllerEmu::Buttons* m_mic;
  ControllerEmu::ControlGroup* m_options;
  ControllerEmu::BooleanSetting* m_always_connected;

  ControllerEmu::ControlGroup* m_primehack_camera;
  ControllerEmu::ControlGroup* m_primehack_misc;
  ControllerEmu::AnalogStick* m_primehack_stick;
  ControllerEmu::PrimeHackModes* m_primehack_modes;

  ControllerEmu::NumericSetting* m_primehack_camera_sensitivity;
  ControllerEmu::NumericSetting* m_primehack_horizontal_sensitivity;
  ControllerEmu::NumericSetting* m_primehack_vertical_sensitivity;

  ControllerEmu::NumericSetting* m_primehack_fieldofview;
  ControllerEmu::BooleanSetting* m_primehack_invert_y;
  ControllerEmu::BooleanSetting* m_primehack_invert_x;

  const unsigned int m_index;

  // Default analog stick radius for GameCube controllers.
  static constexpr ControlState DEFAULT_PAD_STICK_RADIUS = 1.0;
};
