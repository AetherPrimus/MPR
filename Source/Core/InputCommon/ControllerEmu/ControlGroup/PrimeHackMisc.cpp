#include "InputCommon/ControllerEmu/ControlGroup/PrimeHackMisc.h"

namespace ControllerEmu
{
  PrimeHackMisc::PrimeHackMisc(const std::string& name_) : ControlGroup(name_, GroupType::PrimeHackMisc)
  {
  }

  int PrimeHackMisc::GetSelectedDevice() const
  {
    return active_device;
  }

  void PrimeHackMisc::SetSelectedDevice(int val)
  {
    active_device = val;
  }
}  // namespace ControllerEmu
