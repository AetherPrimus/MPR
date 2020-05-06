#pragma once

#include "Core/PrimeHack/PrimeMod.h"
#include "Core/PrimeHack/Transform.h"

namespace prime {

class Noclip : public PrimeMod {
public:
  void run_mod(Game game, Region region) override;
  void init_mod(Game game, Region region) override;

private:
  vec3 get_movement_vec(u32 transform_offset);
  void run_mod_mp1(bool has_control);
  void run_mod_mp2(bool has_control);

  void noclip_code_mp1(u32 cplayer_address, u32 start_point, u32 return_location);
  void noclip_code_mp2(u32 cplayer_address, u32 start_point, u32 return_location);

  u32 control_flag_address;
  u32 load_state_address;
  u32 cplayer_address;
  u32 camera_ptr_address;
  u32 camera_offset_address;

  Transform player_tf;
  bool had_control = true;

  // mp2 demands it
  vec3 player_vec;
};

}
