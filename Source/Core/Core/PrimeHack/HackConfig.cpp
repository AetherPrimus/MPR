#include "HackConfig.h"

#include <array>
#include <string>

#include "Common/IniFile.h"
#include "Core/PrimeHack/PrimeUtils.h"
#include "Core/PrimeHack/Mods/FpsControls.h"

#include "InputCommon/ControllerInterface/ControllerInterface.h"

#include "Core/HW/Wiimote.h"
#include "Core/HW/WiimoteEmu/WiimoteEmu.h"
#include "Core/ConfigManager.h"
#include "Core/Config/GraphicsSettings.h"
#include "VideoCommon/VideoConfig.h"

namespace prime
{
  static float sensitivity;
  static float cursor_sensitivity;
  static float camera_fov;

  static std::string device_name, device_source;
  static bool inverted_y = false;
  static bool inverted_x = false;
  static HackManager hack_mgr;
  bool isRunning = false;

  void InitializeHack(std::string const& mkb_device_name, std::string const& mkb_device_source)
  {
    if (!SConfig::GetInstance().bEnablePrimeHack)
    {
      isRunning = false;
      return;
    }

    if (isRunning) return; isRunning = true;

    // Create mods for all games/regions.
    hack_mgr.add_mod(std::make_unique<FpsControls>());
    //hack_mgr.add_mod(std::make_unique<MP1PAL>());
    //hack_mgr.add_mod(std::make_unique<MP2NTSC>());
    //hack_mgr.add_mod(std::make_unique<MP2PAL>());
    //hack_mgr.add_mod(std::make_unique<MP3NTSC>());
    //hack_mgr.add_mod(std::make_unique<MP3PAL>());
    //hack_mgr.add_mod(std::make_unique<MenuNTSC>());
    //hack_mgr.add_mod(std::make_unique<MenuPAL>());

    device_name = mkb_device_name;
    device_source = mkb_device_source;
  }

  bool CheckBeamCtl(int beam_num)
  {
    return Wiimote::CheckBeam(beam_num);
  }

  bool CheckVisorCtl(int visor_num)
  {
    return Wiimote::CheckVisor(visor_num);
  }

  bool CheckVisorScrollCtl(bool direction)
  {
    return Wiimote::CheckVisorScroll(direction);
  }

  bool CheckBeamScrollCtl(bool direction)
  {
    return Wiimote::CheckBeamScroll(direction);
  }

  bool CheckSpringBallCtl()
  {
    return Wiimote::CheckSpringBall();
  }
  
  bool CheckForward() {
    return Wiimote::CheckForward();
  }

  bool CheckBack() {
    return Wiimote::CheckBack();
  }

  bool CheckLeft() {
    return Wiimote::CheckLeft();
  }

  bool CheckRight() {
    return Wiimote::CheckRight();
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

  bool GetBloom()
  {
    return Config::Get(Config::DISABLE_BLOOM);
  }

  bool GetEnableSecondaryGunFX()
  {
    return Config::Get(Config::ENABLE_SECONDARY_GUNFX);
  }

  bool GetAutoArmAdjust()
  {
    return Config::Get(Config::ARMPOSITION_MODE) == 1;
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

  double GetHorizontalAxis()
  {
    if (Wiimote::PrimeUseController())
      return std::get<0>(Wiimote::GetPrimeStickXY());
    else
      return static_cast<double>(g_mouse_input->GetDeltaHorizontalAxis());
  }

  double GetVerticalAxis()
  {
    if (Wiimote::PrimeUseController())
      return std::get<1>(Wiimote::GetPrimeStickXY());
    else
      return static_cast<double>(g_mouse_input->GetDeltaVerticalAxis());
  }

  std::string const& GetCtlDeviceName()
  {
    return device_name;
  }

  std::string const& GetCtlDeviceSource()
  {
    return device_source;
  }

  bool GetCulling()
  {
    return Config::Get(Config::TOGGLE_CULLING);
  }

  HackManager* GetHackManager()
  {
    return &hack_mgr;
  }
}  // namespace prime
