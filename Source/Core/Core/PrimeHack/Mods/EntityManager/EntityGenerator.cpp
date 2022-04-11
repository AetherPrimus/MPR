#include "EntityGenerator.h"

#include <iostream>
#include <filesystem>
#include <picojson/picojson.h>

#include <Core/PrimeHack/Mods/EntityManager/ELFLoader.h>
#include <Common/Timer.h>
#include <Core/ConfigManager.h>

#include "Core/PrimeHack/Mods/EntityManager/Entities/Effect.h"
#include "Core/PrimeHack/Mods/EntityManager/Entities/Light.h"
#include <Common\BitUtils.h>

std::string entity_info_str;
u32 entity_info_time;

namespace prime {

const std::string entities_folder = File::GetSysDirectory() + "/AetherLabs/Entities/";
bool should_update_entities = false;

struct GamePointers {
  u32 state_manager;

  u32 empty_string; 
  u32 alloc_unique_id;
  u32 malloc;
  u32 ensure_world_paks_ready;
  u32 sprintf;
  u32 scripteffect_ct;
  u32 add_object;
  u32 get_object_by_id;

  u32 sin;
  u32 cos;
  u32 tan;

  u32 scripteffect_size;
  u32 phase_offset;
  u32 mlvl_offset;

  u32 gamelight_size;
  u32 gamelight_ct;
  u32 buildcustom;

  bool initialized = false;
} game_pointers;

void EntityGenerator::run_mod(Game game, Region region)
{
  if (game != Game::PRIME_1 && game != Game::PRIME_1_GCN) {
    return;
  }

  // This isn't the most clean solution but it guarantees the ibat and dbat will be correct every frame.
  u32 dbat = PowerPC::ppcState.spr[SPR_DBAT2U];
  u32 ibat = PowerPC::ppcState.spr[SPR_IBAT2U];
  if (!(dbat & (1 << 8)) || !(ibat & (1 << 8))) {
    PowerPC::ppcState.spr[SPR_DBAT2U] |= 0x00000100;
    PowerPC::ppcState.spr[SPR_IBAT2U] |= 0x00000100;
    PowerPC::DBATUpdated();
    PowerPC::IBATUpdated();
  }

  if (ShouldReloadEntities()) {
    Effect::reset_effect_data();
    Effect::load_effects_data(entities_folder);

    Light::reset_light_data();
    Light::load_lights_data(entities_folder);

    ReloadEntitiesConfig(false);

    print_info("The entity configs have been reloaded.", 2000);
  }

  if (entity_info_time > (int) Common::Timer::GetTimeMs()) {
    DevInfo("EntityLoader", entity_info_str.c_str());
  }

  Effect::write_effects_to_ram();
  Light::write_lights_to_ram();

  // Write addresses and values for mod to use.
  write_pointers(game, region);

  if (should_update_entities != EntitiesToggled()) {
    Light::update_light_data(EntitiesToggled());
    Effect::toggle_effects_visibility(EntitiesToggled());
    should_update_entities = EntitiesToggled();
  }

  if (!ELFLoader::executing_mod()) {
    Symbol *sym = g_symbolDB.GetSymbolFromName("output_log");
    if (sym) {
      std::string msg = PowerPC::HostGetString(sym->address, 2048);

      if (msg.length() != 0)
        DevInfo("Output", msg.c_str());
    }
  }
}

bool EntityGenerator::init_mod(Game game, Region region)
{
  ELFLoader::init_elf_loader(game, region);

  if (game != Game::PRIME_1 && game != Game::PRIME_1_GCN) { 
    return true;
  }

  std::string binary = File::GetSysDirectory() + "/AetherLabs/entity_loader.elf";

  if (!File::Exists(binary)) {
    print_info("Couldn't locate entities binary. Is the Sys folder missing?");
    return true;
  }
    
  ELFLoader::load_elf(std::move(binary), this);

  u32 hook_point;
  if (game == Game::PRIME_1_GCN) {
    hook_point = region == Region::NTSC_U ? 0x80048190 : 0; // to fill PAL
  } else /* Trilogy */ {
    hook_point = region == Region::NTSC_U ? 0x8022B93C : 0x8022BC90;
  }

  const u32 wrapper_start_delta = 0x81800000 - hook_point;
  add_code_change(hook_point, 0x48000001 | (wrapper_start_delta & 0x3fffffc));

  Effect::load_effects_data(entities_folder);
  Light::load_lights_data(entities_folder);

  game_pointers.initialized = false;

  return true;
}

void EntityGenerator::write_pointers(Game game, Region region)
{
  if (!game_pointers.initialized) {
    if (!g_symbolDB.GetSymbolFromName("state_manager_addr"))
      return;

    game_pointers.state_manager = get_sym_address("state_manager_addr");

    game_pointers.empty_string = get_sym_address("empty_string_addr");
    game_pointers.alloc_unique_id = get_sym_address("alloc_unique_id_addr");
    game_pointers.malloc = get_sym_address("malloc_addr");
    game_pointers.ensure_world_paks_ready = get_sym_address("ensure_world_paks_ready_addr");
    game_pointers.sprintf = get_sym_address("sprintf_addr");
    game_pointers.scripteffect_ct = get_sym_address("scripteffect_ct_addr");
    game_pointers.add_object = get_sym_address("add_object_addr");
    game_pointers.get_object_by_id = get_sym_address("get_object_by_id_addr");

    game_pointers.sin = get_sym_address("sin_addr");
    game_pointers.cos = get_sym_address("cos_addr");
    game_pointers.tan = get_sym_address("tan_addr");

    game_pointers.scripteffect_size = get_sym_address("scripteffect_size");
    game_pointers.phase_offset = get_sym_address("phase_offset");
    game_pointers.mlvl_offset = get_sym_address("mvlvl_offset");

    game_pointers.gamelight_size = get_sym_address("gamelight_size");
    game_pointers.gamelight_ct = get_sym_address("gamelight_ct_addr");
    game_pointers.buildcustom = get_sym_address("buildcustom_ct_addr");

    game_pointers.initialized = true;
  }

  LOOKUP(state_manager);
  if (game == Game::PRIME_1) {
    // wii world 0x804bfc70
    if (region == Region::NTSC_U) {
      if (read32(game_pointers.state_manager) != state_manager) {
        write32(state_manager, game_pointers.state_manager);
        write32(0x80472A21, game_pointers.empty_string);
        write32(0x802266C0, game_pointers.alloc_unique_id);
        write32(0x80308E0C, game_pointers.malloc);
        write32(0x802AF258, game_pointers.ensure_world_paks_ready);
        write32(0x802b68e8, game_pointers.sprintf);
        write32(0x801E3A7C, game_pointers.scripteffect_ct);
        write32(0x80226AB4, game_pointers.add_object);
        write32(0x8014D9CC, game_pointers.get_object_by_id);

        write32(0x802BBE60, game_pointers.sin);
        write32(0x802BC368, game_pointers.cos);
        write32(0x802BC434, game_pointers.tan);

        write32(0x150, game_pointers.scripteffect_size);
        write32(0x150, game_pointers.gamelight_size);
        write32(0x10, game_pointers.phase_offset);
        write32(0x14, game_pointers.mlvl_offset);

        write32(0x800C8CA4, game_pointers.gamelight_ct);
        write32(0x80304728, game_pointers.buildcustom);
      }
    }
    else /* PAL */ {
      write32(state_manager, game_pointers.state_manager);
      write32(0x80475F39, game_pointers.empty_string);
      write32(0x80226A14, game_pointers.alloc_unique_id);
      write32(0x803090A4, game_pointers.malloc);
      write32(0x802AF5A8, game_pointers.ensure_world_paks_ready);
      write32(0x802B6B2C, game_pointers.sprintf);
      write32(0x801E3DD0, game_pointers.scripteffect_ct);
      write32(0x80226E08, game_pointers.add_object);
      write32(0x8014DB1C, game_pointers.get_object_by_id);

      write32(0x802BC0A4, game_pointers.sin);
      write32(0x802BC5AC, game_pointers.cos);
      write32(0x802BC678, game_pointers.tan);

      write32(0x150, game_pointers.scripteffect_size);
      write32(0x150, game_pointers.gamelight_size);
      write32(0x10, game_pointers.phase_offset);
      write32(0x14, game_pointers.mlvl_offset);

      write32(0x800C8E30, game_pointers.gamelight_ct);
      write32(0x803049C0, game_pointers.buildcustom);
    }
  }
  else if (game == Game::PRIME_1_GCN) {
    if (region == Region::NTSC_U) {
      write32(state_manager, game_pointers.state_manager);
      write32(0x803ccef5, game_pointers.empty_string);
      write32(0x8004d0dc, game_pointers.alloc_unique_id);
      write32(0x8031586c, game_pointers.malloc);
      write32(0x80004810, game_pointers.ensure_world_paks_ready);
      write32(0x8038dcdc, game_pointers.sprintf);
      write32(0x8008F140, game_pointers.scripteffect_ct);
      write32(0x8004cb14, game_pointers.add_object);
      write32(0x8000FE48, game_pointers.get_object_by_id);

      write32(0x80394adc, game_pointers.sin);
      write32(0x803943f0, game_pointers.cos);
      write32(0x80394bb4, game_pointers.tan);

      write32(0x148, game_pointers.scripteffect_size);
      write32(0x148, game_pointers.gamelight_size);
      write32(0x4, game_pointers.phase_offset);
      write32(0x8, game_pointers.mlvl_offset);

      write32(0x800B5824, game_pointers.gamelight_ct);
      write32(0x803063DC, game_pointers.buildcustom);
    }
    else {

    }
  }
}

void print_info(std::string message, u32 display_time) {
  entity_info_str = message;
  entity_info_time = Common::Timer::GetTimeMs() + display_time;
}

const u32 get_sym_address(const std::string& name) {
  return g_symbolDB.GetSymbolFromName(name)->address;
}

// wow i hate this
float ConvertJSONFloat(const std::string& name, const picojson::value& obj)
{
  return Common::FromBigEndian<float>((float)obj.get(name).get<double>());
}

}



