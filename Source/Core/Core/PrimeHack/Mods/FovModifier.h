#pragma once

#include "Core/PrimeHack/PrimeMod.h"

namespace prime {

class FovModifier : public PrimeMod {
public:
  void run_mod(Game game, Region region) override;
  void init_mod(Game game, Region region) override;

private:
  void run_mod_mp1();
  void run_mod_mp2();
  void run_mod_mp3();
};

}
