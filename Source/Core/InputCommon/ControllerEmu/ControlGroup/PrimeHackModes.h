#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "Common/CommonTypes.h"
#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"
#include "InputCommon/ControllerEmu/ControllerEmu.h"

namespace ControllerEmu
{
  class PrimeHackModes : public ControlGroup
  {
  public:
    explicit PrimeHackModes(const std::string& name);

    int GetSelectedDevice() const;
    void SetSelectedDevice(int val);

  private:
    int active_device;
  };
}  // namespace ControllerEmu
