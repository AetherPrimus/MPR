#pragma once

#include <stdint.h>
#include <vector>

#include "Core/PowerPC/PowerPC.h"

namespace prime {

  struct CodeChange {
    uint32_t address, var;
    CodeChange() : CodeChange(0, 0) {}
    CodeChange(uint32_t address, uint32_t var) : address(address), var(var) {}
  };

  enum class Game : int {
    INVALID_GAME = -1,
    MENU = 0,
    PRIME_1 = 1,
    PRIME_2 = 2,
    PRIME_3 = 3,
    PRIME_1_GCN = 4,
    MAX_VAL = PRIME_3,
  };

  enum class Region : int {
    INVALID_REGION = -1,
    NTSC = 0,
    PAL = 1,
    MAX_VAL = PAL,
  };

  enum class ModState {
    // not running, no active instruction changes
    DISABLED,
    // running, no active instruction changes
    CODE_DISABLED,
    // running, active instruction changes
    ENABLED,
  };

  // Skeleton for a game mod
  class PrimeMod {
  public:
    std::vector<CodeChange> const& get_instruction_changes() const {
      if (state == ModState::CODE_DISABLED || state == ModState::DISABLED) {
        return original_instructions;
      }
      else {
        return code_changes;
      }
    }

    // Run the mod, called each time ActionReplay is hit
    virtual void run_mod(Game game, Region region) = 0;
    // Init the mod, called when a new game / new region is loaded
    virtual void init_mod(Game game, Region region) = 0;

    virtual bool should_apply_changes() const {
      std::vector<CodeChange> const *cc_vec;
      if (state == ModState::CODE_DISABLED) {
        cc_vec = &original_instructions;
      }
      else {
        cc_vec = &code_changes;
      }
      
      for (CodeChange const& change : *cc_vec) {
        if (PowerPC::HostRead_U32(change.address) != change.var) {
          return true;
        }
      }
      return false;
    }

    virtual ~PrimeMod() {};

    ModState mod_state() const { return state; }
    void set_state(ModState state) { this->state = state; }
    void save_original_instructions() {
      for (CodeChange const& change : code_changes) {
        original_instructions.emplace_back(change.address,
          PowerPC::HostRead_Instruction(change.address));
      }
    }
    bool has_saved_instructions() const {
      return !original_instructions.empty();
    }
    void reset_mod() {
      code_changes.clear();
      original_instructions.clear();
      initialized = false;
    }
    bool is_initialized() const {
      return initialized;
    }

  protected:
    std::vector<CodeChange> code_changes;
    bool initialized;

  private:
    std::vector<CodeChange> original_instructions;
    ModState state;
  };

}
