#include "Effect.h"

#include "Common/Timer.h"
#include "Core/HW/Memmap.h"
#include "Core/PrimeHack/Mods/EntityManager/EntityGenerator.h"
#include <unordered_set>

namespace prime {
  namespace Effect {
    static std::unordered_set<std::string> paths_processed;
    static std::vector<EffectsWorld> worlds;
    static ScriptEffect effects[512];
    static int effects_count = 0;
    static bool should_reload_effects = true;

    void load_effects_data(std::string path)
    {
      if (!File::Exists(path)) {
        print_info("Cannot find effects path: " + path);
        return;
      }

      for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (entry.is_directory()) {
          load_effects_data(entry.path().string());
        } else if (!entry.path().filename().compare("effects.json")) {
          load_effect_file(entry.path().string());
          paths_processed.insert(entry.path().string());
        }
      }
    }

    void load_effect_file(std::string file)
    {
      if (std::find(paths_processed.begin(), paths_processed.end(), file) != paths_processed.end()) {
        return;
      }

      std::ifstream data (file);

      picojson::value v;
      if (!picojson::parse(v, data).empty()) {
        print_info("Failed to parse effects data config. The mod will not work.");
        return;
      }

      if (!v.is<picojson::object>()) {
        print_info("Effects config is not formatted correctly. The mod will not work.");
        return;
      }

      const picojson::value::object& obj = v.get<picojson::object>(); // Root
      for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) { // Iterate through worlds
        const picojson::value::object& world = i->second.get<picojson::object>(); // Get world
        std::vector<EffectsArea> areas;

        for (picojson::value::object::const_iterator x = world.begin(); x != world.end(); ++x) { // Iterate through areas
          EffectsArea area = EffectsArea(); // Start collecting EffectsArea
          const picojson::value::array& e = x->second.get<picojson::array>(); // Get Effects

          area.area_id = strtoul(x->first.c_str(), 0, 16);

          for (int y = 0; y < e.size(); y++) // Iterate through effects
          {
            ScriptEffect effect = ScriptEffect();

            u32 world_id = strtoul(i->first.c_str(), 0, 16);
            effect.world = Common::swap32(world_id);

            effect.area = Common::swap32(area.area_id);

            u32 partId;
            sscanf(e.at(y).get("partID").to_str().c_str(), "%x", &partId);
            effect.partID = Common::swap32(partId);

            effect.hotInThermal = e[y].get("hotInThermal").evaluate_as_boolean(); 
            effect.thermalVisorVisible = e[y].get("thermalVisorVisible").evaluate_as_boolean();
            effect.combatVisorVisible = e[y].get("combatVisorVisible").evaluate_as_boolean();
            effect.xrayVisorVisible = e[y].get("xrayVisorVisible").evaluate_as_boolean();
            effect.scale_x =  ConvertJSONFloat("scale_x", e[y]);
            effect.scale_y = ConvertJSONFloat("scale_y", e[y]);
            effect.scale_z = ConvertJSONFloat("scale_z", e[y]);
            effect.pos_x = ConvertJSONFloat("pos_x", e[y]);
            effect.pos_y = ConvertJSONFloat("pos_y", e[y]);
            effect.pos_z = ConvertJSONFloat("pos_z", e[y]);
            effect.pitch = ConvertJSONFloat("pitch", e[y]);
            effect.yaw = ConvertJSONFloat("yaw", e[y]);
            effect.roll = ConvertJSONFloat("roll", e[y]);
            effect.dieWhenSystemsDone = e[y].get("dieWhenSystemsDone").evaluate_as_boolean();

            area.effects.emplace_back(effect);
          }

          areas.emplace_back(area);
        }

        u32 world_hex;
        sscanf(i->first.c_str(), "%x", &world_hex);

        worlds.emplace_back(EffectsWorld(world_hex, areas));
      }
    }

    void write_effects_to_ram()
    {
      Symbol* effects_sym = g_symbolDB.GetSymbolFromName("scripteffects");
      if (effects_sym) {
        if (should_reload_effects) {
          effects_count = 0;
          for (auto& world : worlds) {
            for (EffectsArea area : world.areas) {
              for (ScriptEffect effect : area.effects) {      
                effects[effects_count] = effect;
                effects_count++;
              }
            }
          }

          should_reload_effects = false;
        }

        u32 scripteffects_arr = effects_sym->address;

        Memory::CopyToEmu(scripteffects_arr, effects, sizeof(effects));
        write32(effects_count, g_symbolDB.GetSymbolFromName("scripteffect_count")->address); // Amount of effects to load
      }
    }

    void toggle_effects_visibility(bool visible)
    {
      Symbol* addresses_sym = g_symbolDB.GetSymbolFromName("effects_addr_map");
      if (addresses_sym) {
        u32 address = addresses_sym->address;
        for (int i = 0; i < effects_count; i += 1) {
          u32 effect_object = read32(address + (i * 0x8));

          if (effect_object) {
            if (read32(effect_object) == 0x8048c378) { // vtable addr 
              u8 flags;

              if (visible)
                flags = 0xFF;
              else
                flags = 0;

              write8(flags, effect_object + 0xeb);
            }
          }
        }
      }
    }

    void reset_effect_data()
    {
      worlds.clear();
      paths_processed.clear();
      should_reload_effects = true;
    }
  }
}
