#include "HackConfig.h"

#include <array>
#include <string>

#include "Common/IniFile.h"
#include "Core/PrimeHack/PrimeUtils.h"
#include "Core/PrimeHack/Games/MP1.h"
#include "Core/PrimeHack/Games/MP2.h"
#include "Core/PrimeHack/Games/MP3.h"

#include "InputCommon/ControllerInterface/ControllerInterface.h"

#include "Core/HW/Wiimote.h"
#include "Core/HW/WiimoteEmu/WiimoteEmu.h"
#include "Core/ConfigManager.h"
#include "Core/Config/GraphicsSettings.h"
#include "VideoCommon/VideoConfig.h"

namespace prime
{
static std::vector<std::unique_ptr<ControlReference>> control_list;
static float sensitivity;
static float cursor_sensitivity;
static float camera_fov;

static std::string device_name, device_source;
static bool inverted_y = false;
static bool inverted_x = false;
static bool camera_culling = false;
static HackManager hack_mgr;
bool isRunning = false;

void InitializeHack(std::string const& mkb_device_name, std::string const& mkb_device_source)
{
  if (!SConfig::GetInstance().bEnablePrimeHack)
  {
    isRunning = false;
    return;
  }

  if (isRunning)
    return;
  isRunning = true;

  // Create mods for all games/regions.
  hack_mgr.add_mod(std::make_unique<MP1NTSC>());
  hack_mgr.add_mod(std::make_unique<MP1PAL>());
  hack_mgr.add_mod(std::make_unique<MP2NTSC>());
  hack_mgr.add_mod(std::make_unique<MP2PAL>());
  hack_mgr.add_mod(std::make_unique<MP3NTSC>());
  hack_mgr.add_mod(std::make_unique<MP3PAL>());
  hack_mgr.add_mod(std::make_unique<MenuNTSC>());
  hack_mgr.add_mod(std::make_unique<MenuPAL>());

  device_name = mkb_device_name;
  device_source = mkb_device_source;
}

void RefreshControlDevices()
{
  for (int i = 0; i < control_list.size(); i++)
  {
    control_list[i]->UpdateReference(
        g_controller_interface,
        ciface::Core::DeviceQualifier(device_source.c_str(), 0, device_name.c_str()));
  }
}

bool CheckBeamCtl(int beam_num)
{
  return Wiimote::CheckBeam(beam_num);
}

bool CheckVisorCtl(int visor_num)
{
  return Wiimote::CheckVisor(visor_num);
}

bool CheckBeamScrollCtl(bool direction)
{
  return Wiimote::CheckBeamScroll(direction);
}

bool CheckSpringBallCtl()
{
  return Wiimote::CheckSpringBall();
}

void SetEFBToTexture(bool toggle)
{
  return Config::SetCurrent(Config::GFX_HACK_SKIP_EFB_COPY_TO_RAM, toggle);
}

bool UseMPAutoEFB()
{
  return Config::Get(Config::AUTO_EFB);
}

bool GetEFBTexture()
{
  return Config::Get(Config::GFX_HACK_SKIP_EFB_COPY_TO_RAM);
}


bool DisplayInfo()
{
  return Config::Get(Config::TOGGLE_PRIMEHACK_INFO);
}

bool GetBloom()
{
  return Config::Get(Config::DISABLE_BLOOM_PRIME3);
}

bool GetAutoArmAdjust()
{
  return Config::Get(Config::ARMPOSITION_MODE);
}

bool GetToggleArmAdjust()
{
  return Config::Get(Config::TOGGLE_ARM_REPOSITION);
}

std::tuple<float, float, float> GetArmXYZ() {
  float x = Config::Get(Config::ARMPOSITION_LEFTRIGHT) / 100.f;
  float y = Config::Get(Config::ARMPOSITION_FORWARDBACK) / 100.f;
  float z = Config::Get(Config::ARMPOSITION_UPDOWN) / 100.f;

  return std::make_tuple(x, y, z);
}

void UpdateHackSettings()
{
  double camera, cursor, fov;
  bool invertx, inverty;
  std::tie<double, double, double, bool, bool>(camera, cursor, fov, invertx, inverty) =
    Wiimote::PrimeSettings();

  SetSensitivity((float)camera);
  SetCursorSensitivity((float)cursor);
  SetFov((float)fov);
  SetInvertedX(invertx);
  SetInvertedY(inverty);
}

std::vector<std::unique_ptr<ControlReference>>& GetMutableControls()
{
  return control_list;
}

float GetSensitivity()
{
  return sensitivity;
}

void SetSensitivity(float sens)
{
  sensitivity = sens;
}

float GetCursorSensitivity()
{
  return cursor_sensitivity;
}

void SetCursorSensitivity(float sens)
{
  cursor_sensitivity = sens;
}

float GetFov()
{
  return camera_fov;
}

void SetFov(float fov)
{
  camera_fov = fov;
}

bool InvertedY()
{
  return inverted_y;
}

void SetInvertedY(bool inverted)
{
  inverted_y = inverted;
}

bool InvertedX()
{
  return inverted_x;
}

void SetInvertedX(bool inverted)
{
  inverted_x = inverted;
}

void SetCulling(bool culling)
{
  camera_culling = culling; 
}

bool GetCulling()
{
  return Config::Get(Config::TOGGLE_CULLING);
}

std::string const& GetCtlDeviceName()
{
  return device_name;
}

std::string const& GetCtlDeviceSource()
{
  return device_source;
}

HackManager* GetHackManager()
{
  return &hack_mgr;
}
}  // namespace prime
