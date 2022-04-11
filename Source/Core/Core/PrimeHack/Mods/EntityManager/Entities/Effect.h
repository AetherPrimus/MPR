#pragma once

#include "Core/PrimeHack/PrimeUtils.h"

#include <iostream>
#include <filesystem>
#include <picojson/picojson.h>

#define EFFECTS_ADDR 0x81cf0000

namespace prime {
  namespace Effect {
    struct ScriptEffect {
      u32 area;
      u32 world;
      u32 partID;
      bool hotInThermal;
      bool combatVisorVisible;
      bool thermalVisorVisible;
      bool xrayVisorVisible;
      float scale_x, scale_y, scale_z;
      float pos_x, pos_y, pos_z;
      float pitch, yaw, roll;
      bool dieWhenSystemsDone;
      u32 address;
    };


    struct EffectsArea {
      u32 area_id;
      std::vector<ScriptEffect> effects;
    };

    struct EffectsWorld {
      u32 world_id;
      std::vector<EffectsArea> areas;

      EffectsWorld(u32 id, std::vector<EffectsArea> areas) : world_id(id), areas(areas) {};
    };

    void reset_effect_data();
    void load_effects_data(std::string path);
    void load_effect_file(std::string file);
    void write_effects_to_ram();

    void toggle_effects_visibility(bool visible);
  }
}
