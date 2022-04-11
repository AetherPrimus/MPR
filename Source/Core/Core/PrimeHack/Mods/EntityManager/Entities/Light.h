#pragma once

#include "Core/PrimeHack/PrimeUtils.h"

#include <iostream>
#include <filesystem>
#include <picojson/picojson.h>

#define LIGHTS_ADDR 0x81d00000

namespace prime {
  namespace Light {
    struct GameLight {
      u32 area;
      u32 world;
      float pos_x, pos_y, pos_z;
      float pitch, yaw, roll;
      float distC, distL, distQ;
      float angleC, angleL, angleQ;
      u32 color; // Intentionally u32 so no casting is required.
      u32 priority;
    };


    struct LightsArea {
      u32 area_id;
      std::vector<GameLight> lights;
    };

    struct LightsWorld {
      u32 world_id;
      std::vector<LightsArea> areas;

      LightsWorld(u32 id, std::vector<LightsArea> areas) : world_id(id), areas(areas) {};
    };

    void reset_light_data();
    void load_lights_data(std::string path);
    void load_light_file(std::string file);
    void write_lights_to_ram();
    void update_light_data(bool visible);
  }
}
