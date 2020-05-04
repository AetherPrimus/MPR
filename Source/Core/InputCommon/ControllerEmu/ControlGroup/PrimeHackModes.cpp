#include "InputCommon/ControllerEmu/ControlGroup/PrimeHackModes.h"

namespace ControllerEmu
{
  PrimeHackModes::PrimeHackModes(const std::string& name_) : ControlGroup(name_, GroupType::PrimeHackModes)
  {
  }

  int PrimeHackModes::GetSelectedDevice() const
  {
    return active_device;
  }

  void PrimeHackModes::SetSelectedDevice(int val)
  {
    active_device = val;
  }
}  // namespace ControllerEmu
