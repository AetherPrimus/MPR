#pragma once

#include <optional>
#include <map>
#include "Core/PrimeHack/PrimeMod.h"

namespace prime {
class ElfModLoader : public PrimeMod {
public:
  void run_mod(Game game, Region region) override;
  void init_mod(Game game, Region region) override;

private:
  enum class CVarType {
    INT8, INT16, INT32, INT64, FLOAT32, FLOAT64
  };
  struct CVar {
    std::string name;
    u32 addr;
    CVarType type;

    CVar() = default;
    CVar(std::string&& name, u32 addr, CVarType type) : name(std::forward<std::string>(name)), addr(addr), type(type) {}
  };

  std::map<std::string, CVar> cvar_map;
  size_t entry_point_index;

  void update_bat_regs();
  void parse_and_load_modfile(std::string const& path);
  std::optional<ElfModLoader::CVar> parse_cvar(std::string const& str);
  std::optional<CodeChange> parse_code(std::string const& str);
  std::optional<std::string> parse_elfpath(std::string const& rel_file, std::string const& str);

  void run_mod_mp1_gc(Region region);

  bool executing_mod() const;
  void load_elf(std::string const& path);
};
}
