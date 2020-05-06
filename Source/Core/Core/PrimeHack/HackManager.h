#pragma once

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

  std::vector<std::unique_ptr<PrimeMod>> mod_list;
};
  
}
