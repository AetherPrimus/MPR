#pragma once

#include <array>
#include <memory>

#include "Core/PrimeHack/PrimeMod.h"

namespace prime {

// Determines current running game, activates enabled modifications for said game
  class HackManager {
  public:
    HackManager();
    void run_active_mods();
    void add_mod(std::unique_ptr<PrimeMod> mod);
    Game get_active_game() const { return active_game; }
    Region get_active_region() const { return active_region; }

  private:
    Game active_game;
    Region active_region;
    Game last_game;
    Region last_region;

    std::array<std::array<std::vector<std::unique_ptr<PrimeMod>>, REGION_SZ>, GAME_SZ> mod_list;
  };
  
}
