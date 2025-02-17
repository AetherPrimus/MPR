#pragma once

#include <memory>
#include <vector>

#include "Core/PrimeHack/AddressDB.h"
#include "Core/PrimeHack/EmuVariableManager.h"
#include "Core/PrimeHack/HackManager.h"
#include "InputCommon/ControlReference/ControlReference.h"

// Naming scheme will match dolphin as this place acts as an interface between the hack & dolphin proper
namespace prime {
void InitializeHack(std::string const& mkb_device_name, std::string const& mkb_device_source);

bool CheckBeamCtl(int beam_num);
bool CheckVisorCtl(int visor_num);
bool CheckBeamScrollCtl(bool direction);
bool CheckVisorScrollCtl(bool direction);
bool CheckSpringBallCtl();
bool ImprovedMotionControls();
bool CheckForward();
bool CheckBack();
bool CheckLeft();
bool CheckRight();
bool CheckJump();

bool CheckGrappleCtl();
bool GrappleTappingMode();
bool GrappleCtlBound();

void SetEFBToTexture(bool toggle);
bool UseMPAutoEFB();
bool LockCameraInPuzzles();
bool GetRestoreDashing();
bool GetEFBTexture();
bool GetBloom();
bool GetReduceBloom();

bool GetNoclip();
bool GetInvulnerability();
bool GetSkipCutscene();

bool GetEnableSecondaryGunFX();
bool GetShowGCCrosshair();
u32 GetGCCrosshairColor();

bool GetAutoArmAdjust();
bool GetToggleArmAdjust();
std::tuple<float, float, float> GetArmXYZ();

void UpdateHackSettings();

float GetSensitivity();
void SetSensitivity(float sensitivity);
float GetCursorSensitivity();
void SetCursorSensitivity(float sensitivity);
bool ScaleCursorSensitivity();
void SetScaleCursorSensitivity(bool scale);
float GetFov();
bool InvertedY();
void SetInvertedY(bool inverted);
bool InvertedX();
void SetInvertedX(bool inverted);
bool GetCulling();

bool HandleReticleLockOn();
void SetReticleLock(bool lock);

enum CameraLock { Centre, Angle45, Unlocked };

void SetLockCamera(CameraLock lock);
CameraLock GetLockCamera();

double GetHorizontalAxis();
double GetVerticalAxis();

void ReloadEntitiesConfig(bool reload);
void ReloadEntitiesConfig();
bool ShouldReloadEntities();

void ToggleEntities(bool toggle);
void ToggleEntities();
bool EntitiesToggled();

int GetCurrentBeam();
void SetCurrentBeam(int beam);

void ToggleCannonHolster();
bool GetCannonHolster();

std::pair<u32, u32> GetCurrentPosition();
void SetCurrentPosition(u32 w, u32 a);

std::string const& GetCtlDeviceName();
std::string const& GetCtlDeviceSource();

std::tuple<bool, bool> GetMenuOptions();

HackManager *GetHackManager();
AddressDB *GetAddressDB();
EmuVariableManager *GetVariableManager();

bool ModPending();
void ClearPendingModfile();
std::string GetPendingModfile();
void SetPendingModfile(std::string const& path);

bool ModSuspended();
void SuspendMod();
void ResumeMod();

void Shutdown();
}
