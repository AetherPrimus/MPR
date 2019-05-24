#pragma once

#include <memory>
#include <vector>

#include "InputCommon/ControlReference/ControlReference.h"

namespace prime
{
void InitializeHack(std::string const& mkb_device_name, std::string const& mkb_device_source);

// PRECONDITION:  For all following functions, InitializeHack has been called
std::vector<std::unique_ptr<ControlReference>>& GetMutableControls();
void RefreshControlDevices();
void SaveStateToIni();
void LoadStateFromIni();
bool CheckBeamCtl(int beam_num);
bool CheckVisorCtl(int visor_num);

float GetSensitivity();
void SetSensitivity(float sensitivity);
float GetCursorSensitivity();
void SetCursorSensitivity(float sensitivity);
float GetFov();
void SetFov(float fov);

std::string const& GetCtlDeviceName();
std::string const& GetCtlDeviceSource();
}
