// Copyright 2010 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "InputCommon/ControllerInterface/DInput/DInput.h"
#include "Common/StringUtil.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"

#include "InputCommon/ControllerInterface/DInput/DInputJoystick.h"
#include "InputCommon/ControllerInterface/DInput/DInputKeyboardMouse.h"

#include "InputCommon/DInputMouseAbsolute.h"

#include "Common/IniFile.h"
#include "Core/ActionReplay.h"

#pragma comment(lib, "Dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace ciface
{
namespace DInput
{
BOOL CALLBACK DIEnumDeviceObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
  ((std::list<DIDEVICEOBJECTINSTANCE>*)pvRef)->push_back(*lpddoi);
  return DIENUM_CONTINUE;
}

BOOL CALLBACK DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
  ((std::list<DIDEVICEINSTANCE>*)pvRef)->push_back(*lpddi);
  return DIENUM_CONTINUE;
}

std::string GetDeviceName(const LPDIRECTINPUTDEVICE8 device)
{
  DIPROPSTRING str = {};
  str.diph.dwSize = sizeof(str);
  str.diph.dwHeaderSize = sizeof(str.diph);
  str.diph.dwHow = DIPH_DEVICE;

  std::string result;
  if (SUCCEEDED(device->GetProperty(DIPROP_PRODUCTNAME, &str.diph)))
  {
    result = StripSpaces(UTF16ToUTF8(str.wsz));
  }

  return result;
}

void PopulateDevices(HWND hwnd)
{
  IDirectInput8* idi8;
  if (FAILED(DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8,
                                (LPVOID*)&idi8, nullptr)))
  {
    return;
  }
  IniFile sens_file;
  sens_file.Load("config_sensitivity.ini", true);
  float sensitivity;
  if (!sens_file.GetIfExists<float>("mouse", "PrimeHack_Sensitivity", &sensitivity))
  {
    auto* section = sens_file.GetOrCreateSection("mouse");
    section->Set("PrimeHack_Sensitivity", 7.5f);
    sensitivity = 7.5f;
    sens_file.Save("config_sensitivity.ini");
  }

  ActionReplay::SetSensitivity(sensitivity);

  // MODIFICATION: Initialize external mouse device, keep it separate from Dolphin's keyboardmouse
  // devices
  InputExternal::InitMouse(idi8);

  InitKeyboardMouse(idi8, hwnd);
  InitJoystick(idi8, hwnd);

  idi8->Release();
}
}  // namespace DInput
}  // namespace ciface
