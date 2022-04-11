#include "Light.h"

#include "Common/Timer.h"
#include "Core/HW/Memmap.h"
#include "Core/PrimeHack/Mods/EntityManager/EntityGenerator.h"
#include <unordered_set>

namespace prime {
  namespace Light {
    static std::unordered_set<std::string> paths_processed;
    static std::vector<LightsWorld> worlds;
    static GameLight gamelights[512];
    static int gamelight_count = 0;
    static bool should_reload_lights = true;

    void load_lights_data(std::string path)
    {
      if (!File::Exists(path)) {
        print_info("Cannot find lights path: " + path);
        return;
      }

      for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (entry.is_directory()) {
          load_lights_data(entry.path().string());
        } else if (!entry.path().filename().compare("gamelights.json")) {
          load_light_file(entry.path().string());
          paths_processed.insert(entry.path().string());
        }
      }
    }

    void load_light_file(std::string file)
    {
      if (std::find(paths_processed.begin(), paths_processed.end(), file) != paths_processed.end()) {
        return;
      }

      std::ifstream data (file);

      picojson::value v;
      if (!picojson::parse(v, data).empty()) {
        print_info("Failed to parse lights data config. The mod will not work.");
        return;
      }

      if (!v.is<picojson::object>()) {
        print_info("lights config is not formatted correctly. The mod will not work.");
        return;
      }

      // Rewrite needed.
      // - Needs to combine existing worlds / areas
      const picojson::value::object& obj = v.get<picojson::object>(); // Root
      for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) { // Iterate through worlds
        const picojson::value::object& world = i->second.get<picojson::object>(); // Get world
        std::vector<LightsArea> areas;

        for (picojson::value::object::const_iterator x = world.begin(); x != world.end(); ++x) { // Iterate through areas
          LightsArea area = LightsArea(); // Start collecting lightsArea
          const picojson::value::array& e = x->second.get<picojson::array>(); // Get lights

          area.area_id = strtoul(x->first.c_str(), 0, 16);
          
          for (int y = 0; y < e.size(); y++) // Iterate through lights
          {
            GameLight light = GameLight();

            u32 world_id = strtoul(i->first.c_str(), 0, 16);
            light.world = Common::swap32(world_id);

            light.area = Common::swap32(area.area_id);

            light.pos_x = ConvertJSONFloat("pos_x", e[y]);
            light.pos_y = ConvertJSONFloat("pos_y", e[y]);
            light.pos_z = ConvertJSONFloat("pos_z", e[y]);

            light.pitch = ConvertJSONFloat("pitch", e[y]);
            light.yaw   = ConvertJSONFloat("yaw", e[y]);
            light.roll  = ConvertJSONFloat("roll", e[y]);

            light.distC = ConvertJSONFloat("distC", e[y]);
            light.distL = ConvertJSONFloat("distL", e[y]);
            light.distQ = ConvertJSONFloat("distQ", e[y]);

            light.angleC = ConvertJSONFloat("angleC", e[y]);
            light.angleL = ConvertJSONFloat("angleL", e[y]);
            light.angleQ = ConvertJSONFloat("angleQ", e[y]);

            u32 color_hex;
            sscanf(e.at(y).get("color").to_str().c_str(), "%x", &color_hex);
            light.color = Common::swap32(color_hex);

            u32 priority_hex;
            sscanf(e.at(y).get("priority").to_str().c_str(), "%x", &priority_hex);
            light.priority = Common::swap32(priority_hex);

            area.lights.emplace_back(light);
          }

          areas.emplace_back(area);
        }

        u32 world_hex;
        sscanf(i->first.c_str(), "%x", &world_hex);

        worlds.emplace_back(LightsWorld(world_hex, areas));
      }
    }

    void write_lights_to_ram()
    {
      Symbol* lights_sym = g_symbolDB.GetSymbolFromName("gamelights");
      if (lights_sym) {
        if (should_reload_lights) {
          gamelight_count = 0;
          for (auto& world : worlds) {
            for (LightsArea area : world.areas) {
              for (GameLight light : area.lights) {      
                gamelights[gamelight_count] = light;
                gamelight_count++;
              }
            }
          }

          update_light_data(true);

          should_reload_lights = false;
        }

        u32 gamelights_arr = lights_sym->address;

        Memory::CopyToEmu(gamelights_arr, gamelights, sizeof(gamelights));
        write32(gamelight_count, g_symbolDB.GetSymbolFromName("gamelight_count")->address); // Amount of lights to load
      }
    }

    void update_light_data(bool visible)
    {
      Symbol* addresses_sym = g_symbolDB.GetSymbolFromName("lights_addr_map");
      if (addresses_sym) {
        u32 address = addresses_sym->address;
        for (int i = 0; i < gamelight_count; i++) {
          u32 light_object = read32(address + (i * 0x8));

          if (light_object) {
            if (read32(light_object) == 0x8048F518) {
              u32 color = visible ? gamelights[i].color : 0;

              Memory::CopyToEmu(light_object + 0xEC + 0x20, &color, sizeof(u32));
              Memory::CopyToEmu(light_object + 0xEC + 0x2C, &gamelights[i].distC, sizeof(float));
              Memory::CopyToEmu(light_object + 0xEC + 0x30, &gamelights[i].distL, sizeof(float));
              Memory::CopyToEmu(light_object + 0xEC + 0x34, &gamelights[i].distQ, sizeof(float));
              Memory::CopyToEmu(light_object + 0xEC + 0x38, &gamelights[i].angleC, sizeof(float));
              Memory::CopyToEmu(light_object + 0xEC + 0x3C, &gamelights[i].angleL, sizeof(float));
              Memory::CopyToEmu(light_object + 0xEC + 0x40, &gamelights[i].angleQ, sizeof(float));

              Memory::CopyToEmu(light_object + 0xEC + 0x8, &gamelights[i].pos_x, sizeof(float));
              Memory::CopyToEmu(light_object + 0xEC + 0xC, &gamelights[i].pos_y, sizeof(float));
              Memory::CopyToEmu(light_object + 0xEC + 0x10, &gamelights[i].pos_z, sizeof(float));

              Memory::CopyToEmu(light_object + 0xEC + 0x14, &gamelights[i].yaw, sizeof(float));
              Memory::CopyToEmu(light_object + 0xEC + 0x18, &gamelights[i].pitch, sizeof(float));
              Memory::CopyToEmu(light_object + 0xEC + 0x1C, &gamelights[i].roll, sizeof(float));

              Memory::CopyToEmu(light_object + 0xEC + 0x44, &gamelights[i].priority, sizeof(u32));
            }
          }
        }
      }
    }

    void reset_light_data()
    {
      worlds.clear();
      paths_processed.clear();
      should_reload_lights = true;
    }
  }
}
