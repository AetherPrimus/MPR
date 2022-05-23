#pragma once

#include "Core/PrimeHack/PrimeMod.h"
#include "Core/PrimeHack/GameFlags.h"
#include "Core/PrimeHack/HackConfig.h"
#include "Core/PrimeHack/PrimeUtils.h"
#include "Core/PrimeHack/TextureSwapper.h"

#include "VideoCommon/Util/Aether.h"

namespace prime {
  bool HasPhazonSuit = false;

  class InfoTracker : public PrimeMod {
  const char* xyz_format =
    "\n   X: %.3f"
    "\n   Y: %.3f"
    "\n   Z: %.3f";

  public:
    u32 current_world = -1;
    u32 current_area = -1;

    void run_mod(Game game, Region region) override {
      if (region != Region::NTSC_U || game == Game::MENU) {
        return;
      }

      Transform position;

      if (game == Game::PRIME_1_GCN) {
        DevInfo("World_ID", "%08X", read32(read32(mp1_gc_static.world_ptr) + 0x8));
        DevInfo("Area_ID", "%02X", read32(read32(mp1_gc_static.world_ptr) + 0x68));

        LOOKUP(state_manager);
        DevInfo("Object Count", "%d  (max: 1024)", read16(read32(state_manager + 0x810) + 0x200a));
        DevInfo("GameLight Count", "%d (address: %x)", read16(read32(state_manager + 0x828) + 0x200a), read32(state_manager + 0x828));

        position.read_from(mp1_gc_static.cplayer_address + 0x34);
      }

      if (game == Game::PRIME_1) {
        u32 world_id = read32(read32(mp1_static.world_ptr) + 0x14);
        u32 area_id = read32(read32(mp1_static.world_ptr) + 0x6C);
        DevInfo("World_ID", "%08X", world_id);
        DevInfo("Area_ID", "%08X", area_id);

        SetCurrentPosition(world_id, area_id);

        if (world_id != current_world || area_id != current_area) {
          current_world = world_id;
          current_area = area_id;
        }

        LOOKUP(state_manager);
        DevInfo("Object Count", "%d (max: 1024)", read16(read32(state_manager + 0x810) + 0x200a));
        DevInfo("GameLight Count", "%d (address: %x)", read16(read32(state_manager + 0x828) + 0x200a), read32(state_manager + 0x828));

        position.read_from(mp1_static.cplayer_address + 0x2c);
      }

      LOOKUP_DYN(powerups_array);
      LOOKUP(powerups_size);
      LOOKUP(powerups_offset);

      bool previous = HasPhazonSuit;

      HasPhazonSuit =
          read32(powerups_array + 0x4 + (0x17 /* Phazon Suit*/ * powerups_size) + powerups_offset + 0x4 /* Tracked by capacity, not amount */);

      if (previous != HasPhazonSuit)
        Aether::InitPaks();

      vec3 xyz = position.loc();
      DevInfo("Position", xyz_format, xyz.x, xyz.y, xyz.z);
    }

    bool init_mod(Game game, Region region) override {
      if (game == Game::PRIME_1_GCN && region == Region::NTSC_U) {
        mp1_gc_static.world_ptr = 0x8045A9F8;
        mp1_gc_static.cplayer_address = 0x8046b97c;
      }
      else {
        mp1_static.world_ptr = 0x804bfc70;
        mp1_static.cplayer_address = 0x804d3c20;
      }

      return true;
    }

    void on_state_change(ModState old_state) override {}

  private:
    union {
      struct {
        u32 cplayer_address;
        u32 world_ptr;
      } mp1_static;

      struct {
        u32 world_ptr;
        u32 cplayer_address;
      } mp1_gc_static;
    };
  };
}
