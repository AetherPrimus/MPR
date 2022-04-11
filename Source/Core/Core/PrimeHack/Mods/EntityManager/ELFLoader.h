#pragma once

#include "Core/PrimeHack/PrimeMod.h"
#include "Core/Boot/ElfReader.h"
#include "Core/PowerPC/PPCSymbolDB.h"

#include <Windows.h>
#include <wx/filedlg.h>
#include <Core\PrimeHack\HackConfig.h>
#include <Core\PrimeHack\PrimeUtils.h>
#include <Core\PrimeHack\Mods\EntityManager\EntityGenerator.h>
namespace prime {
  namespace ELFLoader {
    bool initialised = false;

    void init_elf_loader(Game game, Region region) {
      if (!initialised) {
        // Patch the IBAT and DBAT register initialisation to increase the mapped size.
        if (game == Game::PRIME_1_GCN) {
          if (region == Region::NTSC_U) {
            write32(0x38a501ff, 0x80382BA0);
          }
          else {
            //write32(0x38a501ff, 0x803C6318);
          }

          PowerPC::ppcState.spr[SPR_DBAT2U] |= 0x00000100;
          PowerPC::ppcState.spr[SPR_IBAT2U] |= 0x00000100;
          PowerPC::DBATUpdated();
          PowerPC::IBATUpdated();
        }
        else {
          if (region == Region::NTSC_U) {
            write32(0x38a501ff, 0x803c4dc4);
          }
          else {
            write32(0x38a501ff, 0x803C6318);
          }      
        }
        

        initialised = true;
      }
    }

    bool executing_mod() {
      return (read32(0x818000a8) || (PC >= 0x81800000));
    }

    void load_elf(std::string const& path, EntityGenerator* generator) {
      ElfReader elf_file(path);
      elf_file.LoadIntoMemory(false);
      g_symbolDB.Clear();
      elf_file.LoadSymbols();

      const u32 value = 0x48000001 | ((elf_file.GetEntryPoint() - 0x81800014) & 0x3fffffc);

      // Mod Executor
      generator->add_code_change(0x81800000, 0x9421FF8C);
      generator->add_code_change(0x81800004, 0x7C0802A6);
      generator->add_code_change(0x81800008, 0x90010078);
      generator->add_code_change(0x8180000C, 0xBC610000);
      generator->add_code_change(0x81800010, 0x7C832378); // Move area from register 4 into register 3
      //generator->add_code_change(0x81800014, 0x48000004); // Will be replaced with bl to mod.
      generator->add_code_change(0x81800014, value);
      generator->add_code_change(0x81800018, 0xB8610000);
      generator->add_code_change(0x8180001C, 0x80010078);
      generator->add_code_change(0x81800020, 0x7C0803A6);
      generator->add_code_change(0x81800024, 0x38210074);
      generator->add_code_change(0x81800028, 0x7CDE3378); // Run hooked instruction
      generator->add_code_change(0x8180002C, 0x4E800020);
    }
  }
}
