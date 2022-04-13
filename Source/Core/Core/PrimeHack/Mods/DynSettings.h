#pragma once

#include "Core/PrimeHack/HackConfig.h"
#include "Core/PrimeHack/PrimeMod.h"
#include "Core/PrimeHack/PrimeUtils.h"

#include "Core/Config/GraphicsSettings.h"
#include "VideoCommon/VideoConfig.h"

namespace prime
{
class DynSettings : public PrimeMod
{
public:
  void run_mod(Game game, Region region) override {
    LOOKUP(state_manager);
    u32 world_id = read32(read32(state_manager + 0x850) + 0x14);

    if (world_id == 0x158EFE17 /* Frigate Orpheon */)
    {
      if (Config::Get(Config::GFX_DISABLE_FOG) != true)
        Config::SetCurrent(Config::GFX_DISABLE_FOG, true);
    }
  };

  bool init_mod(Game game, Region region) override {
    return true;
  }

  void on_state_change(ModState old_state) override {}
};

}  // namespace prime
