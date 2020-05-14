#include "Core/PrimeHack/Mods/FovModifier.h"

#include "Core/PrimeHack/PrimeUtils.h"
#pragma optimize("", off)
namespace prime {

void FovModifier::run_mod(Game game, Region region) {
  switch (game) {
  case Game::PRIME_1:
    run_mod_mp1();
    break;
  case Game::PRIME_2:
    run_mod_mp2();
    break;
  case Game::PRIME_3:
    run_mod_mp3();
    break;
  default:
    break;
  }
}

void FovModifier::disable_culling(u32 start_point) {
  write_invalidate(start_point, 0x38600001);
  write_invalidate(start_point + 0x4, 0x4e800020);
}

void FovModifier::adjust_viewmodel(float fov, u32 arm_address, u32 znear_address, u32 znear_value) {
  float left = 0.25f;
  float forward = 0.30f;
  float up = -0.35f;
  bool apply_znear = false;

  if (GetToggleArmAdjust()) {
    if (GetAutoArmAdjust()) {
      if (fov > 125) {
        left = 0.22f;
        forward = -0.02f;

        apply_znear = true;
      }
      else if (fov >= 75) {
        left = -0.00020000000000000017f * fov + 0.265f;
        forward = -0.005599999999999999f * fov + 0.72f;

        apply_znear = true;
      }
    }
    else {
      std::tie<float, float, float>(left, forward, up) = GetArmXYZ();
      apply_znear = true;
    }
  }

  if (apply_znear) {
    write32(znear_value, znear_address);
  }

  writef32(left, arm_address);
  writef32(forward, arm_address + 0x4);
  writef32(up, arm_address + 0x8);
}

void FovModifier::run_mod_mp1() {
  const u32 camera_ptr = read32(mp1_static.camera_ptr_address);
  const u32 camera_offset = ((read32(
    mp1_static.active_camera_offset_address) >> 16) & 0x3ff) << 3;
  const u32 camera_address = read32(camera_ptr + camera_offset + 4);

  const float fov = std::min(GetFov(), 170.f);
  writef32(fov, camera_address + 0x164);
  writef32(fov, mp1_static.global_fov1_address);
  writef32(fov, mp1_static.global_fov2_address);

  adjust_viewmodel(fov, mp1_static.gun_pos_address, camera_address + 0x168,
    0x3d200000);

  if (GetCulling() || GetFov() > 101.f) {
    disable_culling(mp1_static.culling_address);
  }

  DevInfo("Camera_Addr", "%08X", camera_address);
}

void FovModifier::run_mod_mp2() {
  u32 camera_ptr = read32(mp2_static.camera_ptr_address);
  if (!mem_check(camera_ptr) || read32(mp2_static.load_state_address) != 1) {
    return;
  }
  u32 camera_offset =
    ((read32(read32(mp2_static.camera_offset_address)) >> 16) & 0x3ff) << 3;
  u32 camera_offset_tp =
    ((read32(read32(mp2_static.camera_offset_address) + 0x0a) >> 16) & 0x3ff)
    << 3;
  u32 camera_base = read32(camera_ptr + camera_offset + 4);
  u32 camera_base_tp = read32(camera_ptr + camera_offset_tp + 4);
  const float fov = std::min(GetFov(), 170.f);
  writef32(fov, camera_base + 0x1e8);
  writef32(fov, camera_base_tp + 0x1e8);

  adjust_viewmodel(fov, read32(read32(mp2_static.tweakgun_ptr_address)) + 0x4c,
    camera_base + 0x1c4, 0x3d200000);

  if (GetCulling() || GetFov() > 101.f) {
    disable_culling(mp2_static.culling_address);
  }

  DevInfo("Camera_Base", "%08X", camera_base);
}

void FovModifier::run_mod_mp3() {
  u32 camera_fov = read32(
    read32(
      read32(read32(mp3_static.camera_ptr_address) + 0x1010) + 0x1c) +
    0x178);
  u32 camera_fov_tp = read32(
    read32(
      read32(read32(mp3_static.camera_ptr_address) + 0x1010) + 0x24) +
    0x178);

  const float fov = std::min(GetFov(), 170.f);
  writef32(fov, camera_fov + 0x1c);
  writef32(fov, camera_fov_tp + 0x1c);
  writef32(fov, camera_fov + 0x18);
  writef32(fov, camera_fov_tp + 0x18);

  u32 cgame_camera = read32(
    read32(
      read32(
        read32(camera_fov + 0x68) + 0x0c) + 0x18) + 0x14);

  adjust_viewmodel(fov, read32(read32(mp3_static.tweakgun_address))
    + 0xe0, cgame_camera + 0x8C, 0x3dcccccd);

  if (GetCulling() || GetFov() > 96.f) {
    disable_culling(mp3_static.culling_address);
  }

  DevInfo("CGame_Camera", "%08X", cgame_camera);
}

void FovModifier::init_mod(Game game, Region region) {
  switch (game) {
  case Game::PRIME_1:
    init_mod_mp1(region);
    break;
  case Game::PRIME_2:
    init_mod_mp2(region);
    break;
  case Game::PRIME_3:
    init_mod_mp3(region);
    break;
  }
  initialized = true;
}

void FovModifier::init_mod_mp1(Region region) {
  if (region == Region::NTSC) {
    mp1_static.camera_ptr_address = 0x804bfc30;
    mp1_static.active_camera_offset_address = 0x804c4a08;
    mp1_static.global_fov1_address = 0x805c0e38;
    mp1_static.global_fov2_address = 0x805c0e3c;
    mp1_static.gun_pos_address = 0x804ddae4;
    mp1_static.culling_address = 0x802c7dbc;
  }
  else if (region == Region::PAL) {
    mp1_static.camera_ptr_address = 0x804c3b70;
    mp1_static.active_camera_offset_address = 0x804c8948;
    mp1_static.global_fov1_address = 0x805c5178;
    mp1_static.global_fov2_address = 0x805c517c;
    mp1_static.gun_pos_address = 0x804e1a24;
    mp1_static.culling_address = 0x802c8024;
  }
  else {}

}

void FovModifier::init_mod_mp2(Region region) {
  if (region == Region::NTSC) {
    mp2_static.camera_ptr_address = 0x804e7af8;
    mp2_static.camera_offset_address = 0x804eb9ac;
    mp2_static.tweakgun_ptr_address = 0x805cb274;
    mp2_static.culling_address = 0x802c8114;
    mp2_static.load_state_address = 0x804e8824;
  }
  else if (region == Region::PAL) {
    mp2_static.camera_ptr_address = 0x804eef48;
    mp2_static.camera_offset_address = 0x804f2f4c;
    mp2_static.tweakgun_ptr_address = 0x805d2cdc;
    mp2_static.culling_address = 0x802ca730;
    mp2_static.load_state_address = 0x804efc74;
  }
  else {}
}

void FovModifier::init_mod_mp3(Region region) {
  if (region == Region::NTSC) {
    mp3_static.camera_ptr_address = 0x805c6c68;
    mp3_static.tweakgun_address = 0x8066f87c;
    mp3_static.culling_address = 0x8031490c;
  }
  else if (region == Region::PAL) {
    mp3_static.camera_ptr_address = 0x805ca0e8;
    mp3_static.tweakgun_address = 0x806730fc;
    mp3_static.culling_address = 0x80314038;
  }
  else {}
}
}
#pragma optimize("", on)
