#pragma once

#include "Core/PrimeHack/PrimeMod.h"

#include <cmath>
#include <sstream>

#include "Core/PowerPC/PowerPC.h"
#include "Core/PrimeHack/HackConfig.h"
#include "InputCommon/GenericMouse.h"
#include "VideoCommon/RenderBase.h"
#include "VideoCommon/VideoCommon.h"

extern std::string info_str;

namespace prime
{
constexpr u8 read8(u32 addr);
constexpr u16 read16(u32 addr);
constexpr u32 read32(u32 addr);
constexpr u32 readi(u32 addr);
constexpr u64 read64(u32 addr);
constexpr void write8(u8 var, u32 addr);
constexpr void write16(u16 var, u32 addr);
constexpr void write32(u32 var, u32 addr);
constexpr void write64(u64 var, u32 addr);
constexpr void writef32(float var, u32 addr);

constexpr float TURNRATE_RATIO = 0.00498665500569808449206349206349f;

int get_beam_switch(std::array<int, 4> const& beams);
std::tuple<int, int> get_visor_switch(std::array<std::tuple<int, int>, 4> const& visors, bool combat_visor);

void handle_cursor(u32 x_address, u32 y_address, float right_bound, float bottom_bound);

void springball_code(u32 base_offset, std::vector<CodeChange>* code_changes);
void springball_check(u32 ball_address, u32 movement_address);

bool mem_check(u32 address);
void write_invalidate(u32 address, u32 value);
void write_if_different(u32 address, u32 value);
float getAspectRatio();

void set_beam_owned(int index, bool owned);
void set_visor_owned(int index, bool owned);
void set_cursor_pos(float x, float y);

void disable_culling(u32 address);
void adjust_viewmodel(float fov, u32 arm_address, u32 znear_address, u32 znear_value);

void DevInfo(std::string line, u32 hex);
std::string GetDevInfo();
void ClrDevInfo();

bool noclip_enabled();
void register_noclip_enable(CodeChange enable, CodeChange disable, Game game, Region region);
void toggle_noclip(u32 player_tf_addr);
void toggle_noclip_mp2(u32 player_pos_addr);
void noclip(u32 player_tf_addr, u32 camera_tf_addr, bool has_control);
void noclip_mp2(u32 player_pos_addr, u32 camera_tf_addr, bool has_control);

struct vec3 {
  vec3(float x, float y, float z)
    : x(x), y(y), z(z) {}
  vec3() : vec3(0, 0, 0) {}

  float x, y, z;

  vec3 operator*(float c) const {
    return vec3(x * c, y * c, z * c);
  }

  vec3 operator+(vec3 const& other) const {
    return vec3(x + other.x, y + other.y, z + other.z);
  }

  vec3 operator-() const {
    return vec3(-x, -y, -z);
  }
};
struct Transform {
  float m[3][4];

  vec3 right() const {
    return vec3(m[0][0], m[1][0], m[2][0]);
  }

  vec3 fwd() const {
    return vec3(m[0][1], m[1][1], m[2][1]);
  }

  vec3 up() const {
    return vec3(m[0][2], m[1][2], m[2][2]);
  }

  vec3 loc() const {
    return vec3(m[0][3], m[1][3], m[2][3]);
  }

  void update_loc(vec3 const &l) {
    m[0][3] = l.x;
    m[1][3] = l.y;
    m[2][3] = l.z;
  }

  void read_from(u32 transform_addr) {
    for (int i = 0; i < sizeof(Transform) / 4; i++) {
      const u32 data = PowerPC::HostRead_U32(transform_addr + i * 4);
      m[i / 4][i % 4] = *reinterpret_cast<float const *>(&data);
    }
  }
};

}  // namespace prime
