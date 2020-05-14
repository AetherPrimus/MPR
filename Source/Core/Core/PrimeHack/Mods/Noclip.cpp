#include "Core/PrimeHack/Mods/Noclip.h"

#include "Core/PrimeHack/PrimeUtils.h"
#pragma optimize("", off)
namespace prime {

void Noclip::run_mod(Game game, Region region) {
  switch (game) {
  case Game::PRIME_1:
    run_mod_mp1(read32(control_flag_address) == 1);
    break;
  case Game::PRIME_2:
    run_mod_mp2(read32(control_flag_address) == 1 &&
                read32(load_state_address) == 1);
    break;
  default:
    break;
  }
}

vec3 Noclip::get_movement_vec(u32 camera_tf_addr) {
  Transform camera_tf(camera_tf_addr);

  vec3 movement_vec;
  if (CheckForward()) {
    movement_vec = movement_vec + camera_tf.fwd();
  }
  if (CheckBack()) {
    movement_vec = movement_vec + -camera_tf.fwd();
  }
  if (CheckLeft()) {
    movement_vec = movement_vec + -camera_tf.right();
  }
  if (CheckRight()) {
    movement_vec = movement_vec + camera_tf.right();
  }

  DevInfoMatrix("Camera", camera_tf);

  return movement_vec;
}

void Noclip::run_mod_mp1(bool has_control) {
  if (initial_run) {
    player_tf.read_from(cplayer_address + 0x2c);
    initial_run = false;
  }
  if (!has_control) {
    player_tf.read_from(cplayer_address + 0x2c);
    set_state(ModState::CODE_DISABLED);
    apply_instruction_changes();
    had_control = has_control;
    return;
  }
  if (has_control && !had_control) {
    set_state(ModState::ENABLED);
    had_control = has_control;
    return;
  }
  
  const u32 camera_ptr = read32(camera_ptr_address);
  const u32 camera_offset = (((read32(camera_offset_address) + 10) >> 16) & 0x3ff) << 3;
  const u32 camera_tf_addr = read32(camera_ptr + camera_offset + 4) + 0x2c;
  vec3 movement_vec = (get_movement_vec(camera_tf_addr) * 0.5f) + player_tf.loc();

  player_tf.set_loc(movement_vec);
  writef32(movement_vec.x, cplayer_address + 0x2c + 0x0c);
  writef32(movement_vec.y, cplayer_address + 0x2c + 0x1c);
  writef32(movement_vec.z, cplayer_address + 0x2c + 0x2c);
}

void Noclip::run_mod_mp2(bool has_control) {
  const u32 _cplayer_address = read32(cplayer_address);
  if (initial_run) {
    player_tf.read_from(_cplayer_address + 0x2c);
    initial_run = false;
  }
  if (!has_control) {
    player_vec.read_from(_cplayer_address + 0x50);
    set_state(ModState::CODE_DISABLED);
    apply_instruction_changes();
    had_control = has_control;
    return;
  }
  if (has_control && !had_control) {
    set_state(ModState::ENABLED);
    had_control = has_control;
    return;
  }

  player_vec = (get_movement_vec(read32(mp2_camera_ptr_address) + 0x20) * 0.5f) + player_vec;
  player_vec.write_to(_cplayer_address + 0x50);
}

void Noclip::init_mod(Game game, Region region) {
  initialized = true;
  initial_run = true;
  switch (game) {
  case Game::PRIME_1:
    if (region == Region::NTSC) {
      noclip_code_mp1(0x804d3c20, 0x800053a4, 0x8000bb80);
      control_flag_address = 0x8052e9b8;
      cplayer_address = 0x804d3c20;
      camera_ptr_address = 0x804bfc30;
      camera_offset_address = 0x804c4a08;
    }
    else if (region == Region::PAL) {
      noclip_code_mp1(0x804d7b60, 0x800053a4, 0x8000bb80);
      control_flag_address = 0x80532b38;
      cplayer_address = 0x804d7b60;
      camera_ptr_address = 0x804c3b70;
      camera_offset_address = 0x804c8948;
    }
    else {}
    break;
  case Game::PRIME_2:
    if (region == Region::NTSC) {
      noclip_code_mp2(0x804e87dc, 0x800053a4, 0x8000d694);
      control_flag_address = 0x805373f8;
      load_state_address = 0x804e8824;
      cplayer_address = 0x804e87dc;
      mp2_camera_ptr_address = 0x804eb9b4;
    }
    else if (region == Region::PAL) {
      noclip_code_mp2(0x804efc2c, 0x800053a4, 0x8000d694);
      control_flag_address = 0x8053ebf8;
      load_state_address = 0x804efc74;
      cplayer_address = 0x804efc2c;
    }
    else {}
    break;
  default:
    break;
  }
}

void Noclip::noclip_code_mp1(u32 cplayer_address, u32 start_point, u32 return_location) {
  u32 return_delta = return_location - (start_point + 0x14);
  u32 start_delta = start_point - (return_location - 4);
  code_changes.emplace_back(start_point + 0x00, 0x3ca00000 | ((cplayer_address >> 16) & 0xffff));
  code_changes.emplace_back(start_point + 0x04, 0x60a50000 | (cplayer_address & 0xffff));
  code_changes.emplace_back(start_point + 0x08, 0x7c032800);
  code_changes.emplace_back(start_point + 0x0c, 0x4d820020);
  code_changes.emplace_back(start_point + 0x10, 0x800300e8);
  code_changes.emplace_back(start_point + 0x14, 0x48000000 | (return_delta & 0x3fffffc));
  code_changes.emplace_back(return_location - 4, 0x48000000 | (start_delta & 0x3fffffc));
}

void Noclip::noclip_code_mp2(u32 cplayer_address, u32 start_point, u32 return_location) {
  u32 return_delta = return_location - (start_point + 0x18);
  u32 start_delta = start_point - (return_location - 4);
  code_changes.emplace_back(start_point + 0x00, 0x3ca00000 | ((cplayer_address >> 16) & 0xffff));
  code_changes.emplace_back(start_point + 0x04, 0x60a50000 | (cplayer_address & 0xffff));
  code_changes.emplace_back(start_point + 0x08, 0x80a50000);
  code_changes.emplace_back(start_point + 0x0c, 0x7c032800);
  code_changes.emplace_back(start_point + 0x10, 0x4d820020);
  code_changes.emplace_back(start_point + 0x14, 0xc0040000);
  code_changes.emplace_back(start_point + 0x18, 0x48000000 | (return_delta & 0x3fffffc));
  code_changes.emplace_back(return_location - 4, 0x48000000 | (start_delta & 0x3fffffc));
}
}
