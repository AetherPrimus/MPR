#include "Core/PrimeHack/HackManager.h"

#include "Core/PrimeHack/HackConfig.h"
#include "Core/PrimeHack/PrimeUtils.h"
#include "Core/PowerPC/PowerPC.h"
#include "InputCommon/GenericMouse.h"
#pragma optimize("", off)
namespace prime {
HackManager::HackManager()
  : active_game(Game::INVALID_GAME),
    active_region(Region::INVALID_REGION),
    last_game(Game::INVALID_GAME),
    last_region(Region::INVALID_REGION) {}

void HackManager::run_active_mods() {
  u32 game_sig = PowerPC::HostRead_Instruction(0x80074000);
  switch (game_sig)
  {
  case 0x90010024:
    active_game = Game::MENU;
    active_region = Region::NTSC;
    break;
  case 0x93FD0008:
    active_game = Game::MENU;
    active_region = Region::PAL;
    break;
  case 0x480008D1:
    active_game = Game::PRIME_1;
    active_region = Region::NTSC;
    break;
  case 0x7EE3BB78:
    active_game = Game::PRIME_1;
    active_region = Region::PAL;
    break;
  case 0x7C6F1B78:
    active_game = Game::PRIME_2;
    active_region = Region::NTSC;
    break;
  case 0x90030028:
    active_game = Game::PRIME_2;
    active_region = Region::PAL;
    break;
  case 0x90010020:
    active_game = Game::PRIME_3;
    {
      u32 region_diff = PowerPC::HostRead_U32(0x800CC000);
      if (region_diff == 0x981D005E)
      {
        active_region = Region::NTSC;
      }
      else if (region_diff == 0x8803005D)
      {
        active_region = Region::PAL;
      }
      else
      {
        active_game = Game::INVALID_GAME;
        active_region = Region::INVALID_REGION;
      }
    }
    break;
  default:
    active_game = Game::INVALID_GAME;
    active_region = Region::INVALID_REGION;
  }

  if (active_game != last_game || active_region != last_region) {
    for (auto& mod : mods) {
      mod.second->reset_mod();
    }
  }

  ClrDevInfo(); // Clear the dev info stream before the mods print again.

  update_mod_states();

  if (active_game != Game::INVALID_GAME && active_region != Region::INVALID_REGION) {
    for (auto& mod : mods) {
      if (!mod.second->is_initialized()) {
        mod.second->init_mod(active_game, active_region);
        mod.second->save_original_instructions();
      }
      if (mod.second->should_apply_changes()) {
        mod.second->apply_instruction_changes();
      }
    }
      
    last_game = active_game;
    last_region = active_region;
    for (auto& mod : mods) {
      if (mod.second->mod_state() == ModState::ENABLED ||
          mod.second->mod_state() == ModState::CODE_DISABLED) {
        mod.second->run_mod(active_game, active_region);
      }
    }
  }
  prime::g_mouse_input->ResetDeltas();
}

void HackManager::update_mod_states()
{
  set_mod_enabled("auto_efb", UseMPAutoEFB());
  set_mod_enabled("disable_bloom", GetBloom());
  set_mod_enabled("cut_beam_fx_mp1", GetEnableSecondaryGunFX());
  set_mod_enabled("noclip", GetNoclip());
}

void HackManager::add_mod(std::string const &name, std::unique_ptr<PrimeMod> mod) {
  mods[name] = std::move(mod);
}

void HackManager::disable_mod(std::string const& name) {
  auto result = mods.find(name);
  if (result == mods.end()) {
    return;
  }
  result->second->set_state(ModState::DISABLED);
}

void HackManager::enable_mod(std::string const& name) {
  auto result = mods.find(name);
  if (result == mods.end()) {
    return;
  }
  result->second->set_state(ModState::ENABLED);
}

void HackManager::set_mod_enabled(std::string const& name, bool enabled) {
  if (enabled) {
    enable_mod(name);
  }
  else {
    disable_mod(name);
  }
}

bool HackManager::is_mod_active(std::string const& name) {
  auto result = mods.find(name);
  if (result == mods.end()) {
    return false;
  }
  return result->second->mod_state() != ModState::DISABLED;
}

void HackManager::save_mod_states() {
  for (auto& mod : mods) {
    mod_state_backup[mod.first] = mod.second->mod_state();
  }
}

void HackManager::restore_mod_states() {
  for (auto& mod : mods) {
    auto result = mod_state_backup.find(mod.first);
    if (result == mod_state_backup.end()) {
      continue;
    }
    if (mod.second->is_initialized()) {
      mod.second->set_state(result->second);
    }
  }
}

void HackManager::revert_all_code_changes() {
  for (auto& mod : mods) {
    if (mod.second->is_initialized()) {
      mod.second->set_state(ModState::DISABLED);
      mod.second->apply_instruction_changes();
    }
  }
}
}
