#pragma once

#include "Core/PrimeHack/PrimeMod.h"
#include "Core/Boot/ElfReader.h"
#include "Core/PowerPC/PPCSymbolDB.h"

#include <Windows.h>
#include <wx/filedlg.h>
#include <Core\PrimeHack\HackConfig.h>
#include <Core\PrimeHack\PrimeUtils.h>

#include <picojson/picojson.h>

namespace prime {

  class EntityGenerator : public PrimeMod {
  public:
    void run_mod(Game game, Region region) override;
    bool init_mod(Game game, Region region) override;
    void on_state_change(ModState old_state) override {}

    void write_pointers(Game game, Region region);
  };

  void print_info(std::string message, u32 display_time = 5000);
  const u32 get_sym_address(const std::string& name);
  float ConvertJSONFloat(const std::string& name, const picojson::value& obj);
}

extern std::string entity_info_str;
extern u32 entity_info_time;
