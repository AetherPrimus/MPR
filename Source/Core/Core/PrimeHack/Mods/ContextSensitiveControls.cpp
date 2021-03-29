#include "Core/PrimeHack/Mods/ContextSensitiveControls.h"

#include "Core/PrimeHack/PrimeUtils.h"
#include "Core/PrimeHack/HackConfig.h"

namespace prime {

void ContextSensitiveControls::run_mod(Game game, Region region) {
  // Always reset the camera lock or it will never be unlocked
  // This is done before the game check just in case somebody quits during a motion puzzle.
  SetLockCamera(Unlocked);

  if (game != Game::PRIME_3 && game != Game::PRIME_3_STANDALONE) {
    return;
  }

  LOOKUP_DYN(object_list);
  LOOKUP(motion_vf);
  u32 obj_list_iterator = object_list + 4;

  for (int i = 0; i < 2048; i++) {
    u32 entity = read32(obj_list_iterator);
    u32 entity_flags = read32(entity + 0x38);

    bool should_process = false;
    if (entity_flags & 0x20000000) {
      should_process = ((entity_flags >> 8) & 0x2000) == 0;
    }
    should_process |= ((entity_flags >> 8) & 0x1000) != 0;

    if (should_process) {
      u32 vf_table = read32(entity);
      u32 vft_func = read32(vf_table + 0xc);

      // Accept function for this specific object type ("RTTI" checking)
      if (vft_func == motion_vf) {
        u32 puzzle_state = read32(entity + 0x14c);

        if (ImprovedMotionControls()) {
          if (puzzle_state == 3) {
            float step = readf32(entity + 0x154);

            if (CheckForward()) {
              step += 0.05f;
            }
            if (CheckBack()) {
              step -= 0.05f;
            }

            writef32(std::clamp(step, 0.f, 1.f), entity + 0x154);
          }
        }

        if (puzzle_state > 0) {
          DevInfo("Puzzle Info", "addr: %x (state: %x)", entity, puzzle_state);
        }

        if (LockCameraInPuzzles()) {
          if (puzzle_state > 0) {
            // Clear the first byte, since we do not account for the layer ID.
            u32 id = read32(entity + 0xC) & ~(0xFF << 24);
            // If the id isn't ship radios
            if (blacklisted_editor_ids.find(id) == blacklisted_editor_ids.end()) {
              SetLockCamera(Centre);
            }
            else {
              // Aim up to face the radio properly.
              SetLockCamera(Angle45);
            }
          }
        }  
      } else if (vft_func == motion_vf + 0x38) {
        // If the vf is for rotary, confirm this needs to be controlled
        if (read32(entity + 0x204) == 1) {
          float velocity = 0;
          if (CheckRight()) {
            velocity = 0.04f;
          }
          if (CheckLeft()) {
            velocity -= 0.04f;
          }
          prime::GetVariableManager()->set_variable("rotary_velocity", velocity);
        }
      }
    }

    u16 next_id = read16(obj_list_iterator + 6);
    if (next_id == 0xffff) {
      break;
    }

    obj_list_iterator = ((object_list + 4) + next_id * 8);
  }
}

bool ContextSensitiveControls::init_mod(Game game, Region region) {
  u32 lis, ori;
  prime::GetVariableManager()->register_variable("rotary_velocity");
  std::tie<u32, u32>(lis, ori) = prime::GetVariableManager()->make_lis_ori(12, "rotary_velocity");

  switch (game) {
  case Game::PRIME_3:
    if (region == Region::NTSC_U) {
      // Take control of the rotary puzzles
      add_code_change(0x801f806c, lis);
      add_code_change(0x801f8074, ori);
      add_code_change(0x801f807c, 0xc02c0000);
    } else if (region == Region::PAL) {
      add_code_change(0x801f7b4c, lis);
      add_code_change(0x801f7b54, ori);
      add_code_change(0x801f7b5c, 0xc02c0000);
    }
    break;
  case Game::PRIME_3_STANDALONE:
    if (region == Region::NTSC_U) {
      add_code_change(0x801fb544, lis);
      add_code_change(0x801fb54c, ori);
      add_code_change(0x801fb554, 0xc02c0000);
    } else if (region == Region::NTSC_J) {
      add_code_change(0x801fdb5c, lis);
      add_code_change(0x801fdb64, ori);
      add_code_change(0x801fdb6c, 0xc02c0000);
    } else if (region == Region::PAL) {
      add_code_change(0x801fc5a8, lis);
      add_code_change(0x801fc5b0, ori);
      add_code_change(0x801fc5b8, 0xc02c0000);
    }
    break;
  default:
    break;
  }
  return true;
}
}
