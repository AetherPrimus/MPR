#include "HackConfig.h"

#include <array>
#include <string>

#include "Common/IniFile.h"
#include "Core/PrimeHack/PrimeUtils.h"
#include "Core/PrimeHack/EmuVariableManager.h"

#include "Core/PrimeHack/Mods/AutoEFB.h"
#include "Core/PrimeHack/Mods/CutBeamFxMP1.h"
#include "Core/PrimeHack/Mods/DisableBloom.h"
#include "Core/PrimeHack/Mods/FpsControls.h"
#include "Core/PrimeHack/Mods/Invulnerability.h"
#include "Core/PrimeHack/Mods/ElfModLoader.h"
#include "Core/PrimeHack/Mods/Noclip.h"
#include "Core/PrimeHack/Mods/SkipCutscene.h"
#include "Core/PrimeHack/Mods/SpringballButton.h"
#include "Core/PrimeHack/Mods/ViewModifier.h"
#include "Core/PrimeHack/Mods/ContextSensitiveControls.h"
#include "Core/PrimeHack/Mods/PortalSkipMP2.h"
#include "Core/PrimeHack/Mods/FriendVouchers.h"
#include "Core/PrimeHack/Mods/RestoreDashing.h"
#include "Core/PrimeHack/Mods/DisableHudMemoPopup.h"
#include "Core/PrimeHack/Mods/HudAdjuster.h"
#include "Core/PrimeHack/Mods/DynSettings.h"
#include "Core/PrimeHack/Mods/EntityManager/EntityGenerator.h"
#include "Core/PrimeHack/Mods/EntityManager/InfoTracker.h"

#include "InputCommon/ControllerInterface/ControllerInterface.h"

#include "Core/HW/Wiimote.h"
#include "Core/HW/WiimoteEmu/WiimoteEmu.h"
#include "Core/HW/GCPad.h"
#include "Core/ConfigManager.h"
#include "Core/Config/GraphicsSettings.h"
#include "VideoCommon/VideoConfig.h"

namespace prime {
namespace {
float sensitivity;
float cursor_sensitivity;

std::string device_name, device_source;
bool inverted_x = false;
bool inverted_y = false;
bool scale_cursor_sens = false;
HackManager hack_mgr;
AddressDB addr_db;
EmuVariableManager var_mgr;
bool is_running = false;
CameraLock lock_camera = Unlocked;
bool reticle_lock = false;

std::string pending_modfile = "";
bool mod_suspended = false;
bool reload_entities = false;
bool toggle_entities = true;
bool holster_cannon = false;
int current_beam = 0;

u32 world = -1;
u32 area = -1;
}

void InitializeHack(std::string const& mkb_device_name, std::string const& mkb_device_source) {
  if (is_running) return; is_running = true;
  PrimeMod::set_hack_manager(GetHackManager());
  PrimeMod::set_address_database(GetAddressDB());
  init_db(*GetAddressDB());

  // Create all mods
  hack_mgr.add_mod("auto_efb", std::make_unique<AutoEFB>());
  hack_mgr.add_mod("cut_beam_fx_mp1", std::make_unique<CutBeamFxMP1>());
  hack_mgr.add_mod("bloom_modifier", std::make_unique<DisableBloom>());
  hack_mgr.add_mod("fps_controls", std::make_unique<FpsControls>());
  hack_mgr.add_mod("invulnerability", std::make_unique<Invulnerability>());
  hack_mgr.add_mod("noclip", std::make_unique<Noclip>());
  hack_mgr.add_mod("skip_cutscene", std::make_unique<SkipCutscene>());
  hack_mgr.add_mod("restore_dashing", std::make_unique<RestoreDashing>());
  hack_mgr.add_mod("springball_button", std::make_unique<SpringballButton>());
  hack_mgr.add_mod("fov_modifier", std::make_unique<ViewModifier>());
  hack_mgr.add_mod("elf_mod_loader", std::make_unique<ElfModLoader>());
  hack_mgr.add_mod("context_sensitive_controls", std::make_unique<ContextSensitiveControls>());
  hack_mgr.add_mod("portal_skip_mp2", std::make_unique<PortalSkipMP2>());
  hack_mgr.add_mod("friend_vouchers_cheat", std::make_unique<FriendVouchers>());
  hack_mgr.add_mod("disable_hudmemo_popup", std::make_unique<DisableHudMemoPopup>());

  hack_mgr.add_mod("entity_generator", std::make_unique<EntityGenerator>());
  hack_mgr.add_mod("hud_tweaker", std::make_unique<HudAdjuster>());
  hack_mgr.add_mod("infotracker", std::make_unique<InfoTracker>());
  hack_mgr.add_mod("dynsettings", std::make_unique<DynSettings>());

  device_name = mkb_device_name;
  device_source = mkb_device_source;

  hack_mgr.enable_mod("fov_modifier");
  hack_mgr.enable_mod("skip_cutscene");
  hack_mgr.enable_mod("bloom_modifier");
  hack_mgr.enable_mod("entity_generator");
  hack_mgr.enable_mod("infotracker");
  hack_mgr.enable_mod("hud_tweaker");
  hack_mgr.enable_mod("dynsettings");

  // Enable no PrimeHack control mods
  if (!SConfig::GetInstance().bEnablePrimeHack) {
    return;
  }

  hack_mgr.enable_mod("fps_controls");
  hack_mgr.enable_mod("springball_button");
  hack_mgr.enable_mod("context_sensitive_controls");
  hack_mgr.enable_mod("elf_mod_loader");
}

bool CheckBeamCtl(int beam_num) {
  return Wiimote::CheckBeam(beam_num);
}

bool CheckVisorCtl(int visor_num) {
  return Wiimote::CheckVisor(visor_num);
}

bool CheckVisorScrollCtl(bool direction) {
  return Wiimote::CheckVisorScroll(direction);
}

bool CheckBeamScrollCtl(bool direction) {
  return Wiimote::CheckBeamScroll(direction);
}

bool CheckSpringBallCtl() {
  return Wiimote::CheckSpringBall();
}

bool ImprovedMotionControls() {
  return Wiimote::CheckImprovedMotion();
}
  
bool CheckForward() {
  if (hack_mgr.get_active_game() >= Game::PRIME_1_GCN) {
    return Pad::CheckForward();
  }
  else {
    return Wiimote::CheckForward();
  }
}

bool CheckBack() {
  if (hack_mgr.get_active_game() >= Game::PRIME_1_GCN) {
    return Pad::CheckBack();
  }
  else {
    return Wiimote::CheckBack();
  }
}

bool CheckLeft() {
  if (hack_mgr.get_active_game() >= Game::PRIME_1_GCN) {
    return Pad::CheckLeft();
  }
  else {
    return Wiimote::CheckLeft();
  }
}

bool CheckRight() {
  if (hack_mgr.get_active_game() >= Game::PRIME_1_GCN) {
    return Pad::CheckRight();
  }
  else {
    return Wiimote::CheckRight();
  }
}

bool CheckJump() {
  if (hack_mgr.get_active_game() >= Game::PRIME_1_GCN) {
    return Pad::CheckJump();
  }
  else {
    return Wiimote::CheckJump();
  }
}

bool CheckGrappleCtl() {
  return Wiimote::CheckGrapple();
}

bool GrappleTappingMode() {
  return Wiimote::UseGrappleTapping();
}

bool GrappleCtlBound() {
  return Wiimote::GrappleCtlBound();
}

void SetEFBToTexture(bool toggle) {
  return Config::SetCurrent(Config::GFX_HACK_SKIP_EFB_COPY_TO_RAM, toggle);
}

bool UseMPAutoEFB() {
  return Config::Get(Config::AUTO_EFB);
}

bool LockCameraInPuzzles() {
  return Config::Get(Config::LOCKCAMERA_IN_PUZZLES);
}

bool GetNoclip() {
  return SConfig::GetInstance().bPrimeNoclip;
}

bool GetInvulnerability() {
  return SConfig::GetInstance().bPrimeInvulnerability;
}

bool GetSkipCutscene() {
  return SConfig::GetInstance().bPrimeSkipCutscene;
}

bool GetRestoreDashing() {
  return SConfig::GetInstance().bPrimeRestoreDashing;
}

bool GetEFBTexture() {
  return Config::Get(Config::GFX_HACK_SKIP_EFB_COPY_TO_RAM);
}

bool GetBloom() {
  return Config::Get(Config::DISABLE_BLOOM);
}

bool GetReduceBloom() {
  return Config::Get(Config::REDUCE_BLOOM);
}

bool GetEnableSecondaryGunFX() {
  return Config::Get(Config::ENABLE_SECONDARY_GUNFX);
}

bool GetShowGCCrosshair() {
  return Config::Get(Config::GC_SHOW_CROSSHAIR);
}

u32 GetGCCrosshairColor() {
  return Config::Get(Config::GC_CROSSHAIR_COLOR_RGBA);
}

bool GetAutoArmAdjust() {
  return Config::Get(Config::ARMPOSITION_MODE) == 1;
}

bool GetToggleArmAdjust() {
  return Config::Get(Config::TOGGLE_ARM_REPOSITION);
}

std::tuple<float, float, float> GetArmXYZ() {
  float x = Config::Get(Config::ARMPOSITION_LEFTRIGHT) / 100.f;
  float y = Config::Get(Config::ARMPOSITION_FORWARDBACK) / 100.f;
  float z = Config::Get(Config::ARMPOSITION_UPDOWN) / 100.f;

  return std::make_tuple(x, y, z);
}

void UpdateHackSettings() {
  double camera, cursor;
  bool invertx, inverty, scale_sens = false, reticle_lock = false;

  if (hack_mgr.get_active_game() == Game::PRIME_1_GCN)
    std::tie<double, double, bool, bool>(camera, cursor, invertx, inverty) =
      Pad::PrimeSettings();
  else
    std::tie<double, double, bool, bool, bool>(camera, cursor, invertx, inverty, scale_sens, reticle_lock) =
      Wiimote::PrimeSettings();

  SetSensitivity((float)camera);
  SetCursorSensitivity((float)cursor);
  SetInvertedX(invertx);
  SetInvertedY(inverty);
  SetScaleCursorSensitivity(scale_sens);
  SetReticleLock(reticle_lock);
}

float GetSensitivity() {
  return sensitivity;
}

void SetSensitivity(float sens) {
  sensitivity = sens;
}

float GetCursorSensitivity() {
  return cursor_sensitivity;
}

void SetCursorSensitivity(float sens) {
  cursor_sensitivity = sens;
}

float GetFov() {
  return Config::Get(Config::FOV);
}

bool InvertedY() {
  return inverted_y;
}

void SetInvertedY(bool inverted) {
  inverted_y = inverted;
}

bool InvertedX() {
  return inverted_x;
}

void SetInvertedX(bool inverted) {
  inverted_x = inverted;
}

bool ScaleCursorSensitivity() {
  return scale_cursor_sens;
}

void SetScaleCursorSensitivity(bool scale) {
  scale_cursor_sens = scale;
}

double GetHorizontalAxis() {
  if (hack_mgr.get_active_game() == Game::PRIME_1_GCN) {
    if (Pad::PrimeUseController()) {
      return std::get<0>(Pad::GetPrimeStickXY());
    } 
  }
  else if (Wiimote::PrimeUseController()) {
    return std::get<0>(Wiimote::GetPrimeStickXY());
  } 
  
  return static_cast<double>(g_mouse_input->GetDeltaHorizontalAxis());
}

double GetVerticalAxis() {
  if (hack_mgr.get_active_game() == Game::PRIME_1_GCN) {
    if (Pad::PrimeUseController()) {
      return std::get<1>(Pad::GetPrimeStickXY());
    } 
  }
  else if (Wiimote::PrimeUseController()) {
    return std::get<1>(Wiimote::GetPrimeStickXY());
  } 

  return static_cast<double>(g_mouse_input->GetDeltaVerticalAxis());
}

std::string const& GetCtlDeviceName() {
  return device_name;
}

std::string const& GetCtlDeviceSource() {
  return device_source;
}

bool GetCulling() {
  return Config::Get(Config::TOGGLE_CULLING);
}

bool HandleReticleLockOn()
{
  return reticle_lock;
}

void SetReticleLock(bool lock)
{
  reticle_lock = lock;
}

void SetLockCamera(CameraLock lock) {
  lock_camera = lock;
}

CameraLock GetLockCamera() {
  return lock_camera;
}

std::tuple<bool, bool> GetMenuOptions() {
  return Wiimote::GetBVMenuOptions();
}

void ReloadEntitiesConfig(bool reload = true) {
  reload_entities = reload;
}

void ReloadEntitiesConfig() {
  reload_entities = true;
}

bool ShouldReloadEntities() {
  return reload_entities;
}

void ToggleEntities(bool toggle = true) {
  toggle_entities = toggle;
}

void ToggleEntities() {
  toggle_entities = !toggle_entities;
}

bool EntitiesToggled() {
  return toggle_entities;
}

void SetCurrentBeam(int beam) {
  current_beam = beam;
}

int GetCurrentBeam() {
  return current_beam;
}

void ToggleCannonHolster() {
  holster_cannon = !holster_cannon;
}

bool GetCannonHolster() {
  return holster_cannon;
}

std::pair<u32, u32> GetCurrentPosition() {
  return std::make_pair(world, area);
}

void SetCurrentPosition(u32 w, u32 a) {
  world = w;
  area = a;
}

HackManager* GetHackManager() {
  return &hack_mgr;
}

AddressDB* GetAddressDB() {
  return &addr_db;
}

EmuVariableManager* GetVariableManager() {
  return &var_mgr;
}

bool ModPending() {
  return !pending_modfile.empty();
}

void ClearPendingModfile() {
  pending_modfile.clear();
}

std::string GetPendingModfile() {
  return pending_modfile;
}

void SetPendingModfile(std::string const& path) {
  pending_modfile = path;
}

bool ModSuspended() {
  return mod_suspended;
}

void SuspendMod() {
  mod_suspended = true;
}

void ResumeMod() {
  mod_suspended = false;
}

void Shutdown() {
  hack_mgr.shutdown();
}

}  // namespace prime
