#include "Core/PrimeHack/MemoryWriter.h"

#ifdef _WIN32

#include <Windows.h>

#include <cctype>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <thread>

#include "Core/PowerPC/PowerPC.h"
#include "Common/GekkoDisassembler.h"

namespace prime {
  namespace {
    bool initialized = false;
    std::unique_ptr<std::thread> cli_thread_handle;
  }

  constexpr uint8_t hex_nibble(char c) {
    if (c <= '9' && c >= '0') {
      return c - '0';
    }
    else if (c >= 'a' && c <= 'f') {
      return c - 'a' + 0x0a;
    }
    else if (c >= 'A' && c <= 'F') {
      return c - 'A' + 0x0a;
    }
    return 0;
  }

  // Extract hex out of text and convert to 32 bit word
  uint32_t hex_to_word(std::string_view hex) {
    if (hex[0] == '0' && hex[1] == 'x') {
      hex = hex.substr(2);
    }
    uint32_t ret = 0;
    for (int i = 0; i < 8; i++) {
      ret <<= 4;
      ret |= hex_nibble(hex[i]);
    }
    return ret;
  }

  void handle_write(std::vector<uint32_t> const& params) {
    if (params.size() < 2) {
      std::cout << "Not enough parameters for write" << std::endl;
      return;
    }
    uint32_t write_addr = params[0];
    if (write_addr < 0x80000000 || write_addr >= 0x81800000) {
      std::cout << "Invalid address range to write" << std::endl;
      return;
    }
    const bool needs_invalidate = PowerPC::HostIsInstructionRAMAddress(write_addr);
    for (auto i = 1u; i < params.size(); i++) {
      PowerPC::HostWrite_U32(params[i], write_addr);
      if (needs_invalidate) {
        PowerPC::ScheduleInvalidateCacheThreadSafe(write_addr);
      }
      write_addr += 0x4u;
    }
    std::cout << "Successfully wrote " << params.size() - 1 << " words" << std::endl;;
  }

  void handle_hdump(uint32_t address, int num_words) {
    if (num_words <= 0) {
      std::cout << "Invalid number of words specified" << std::endl;
    }
    if (address < 0x80000000 || address >= 0x81800000) {
      std::cout << "Invalid address range to read" << std::endl;
      return;
    }
    const bool text_section = PowerPC::HostIsInstructionRAMAddress(address);
    for (int i = 0; i < num_words; i++) {
      u32 var = PowerPC::HostRead_U32(address);
      printf("%s:%08X  |  %08X\n", (text_section ? ".text" : ".data"), address, var);
      address += 0x4u;
    }
  }

  void handle_idump(uint32_t address, int num_instructions) {
    if (num_instructions <= 0) {
      std::cout << "Invalid number of instructions specified" << std::endl;
    }
    if (address < 0x80000000 || address >= 0x81800000) {
      std::cout << "Invalid address range to read" << std::endl;
      return;
    }
    const bool text_section = PowerPC::HostIsInstructionRAMAddress(address);
    for (int i = 0; i < num_instructions; i++) {
      std::string instruction = GekkoDisassembler::Disassemble(
        PowerPC::HostRead_U32(address), address);
      printf("%s:%08X  |  %s\n", (text_section ? ".text" : ".data"), address, instruction.c_str());
      address += 0x4u;
    }
  }

  void handle_fdump(uint32_t address, int num_floats) {
    if (num_floats <= 0) {
      std::cout << "Invalid number of floats specified" << std::endl;
    }
    if (address < 0x80000000 || address >= 0x81800000) {
      std::cout << "Invalid address range to read" << std::endl;
      return;
    }
    const bool text_section = PowerPC::HostIsInstructionRAMAddress(address);
    for (int i = 0; i < num_floats; i++) {
      u32 var_hex = PowerPC::HostRead_U32(address);
      float var = *reinterpret_cast<float *>(&var_hex);
      printf("%s:%08X  |  %f\n", (text_section ? ".text" : ".data"), address, var);
      address += 0x4u;
    }
  }

  void cli_thread() {
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);

    const std::regex write_memory("^write( +(0x)?([a-fA-F0-9]{8}))+ *$");
    const std::regex print_hex("^hdump +(0x)?([a-fA-F0-9]{8}) +(\\d+) *$");
    const std::regex print_instructions("^idump +(0x)?([a-fA-F0-9]{8}) +(\\d+) *$");
    const std::regex print_float("^fdump +(0x)?([a-fA-F0-9]{8}) +(\\d+) *$");
    while (true) {
      std::string command;
      std::smatch match;
      std::getline(std::cin, command);

      if (std::regex_search(command, match, write_memory)) {
        // params[0] = address, params[1+] = writes
        std::vector<uint32_t> params;

        std::string const& all_params = command.substr(5) + " ";
        int start = -1;
        for (int i = 0; i < all_params.length(); i++) {
          if (!std::isspace(all_params[i]) && start == -1) {
            start = i;
          }
          else if (std::isspace(all_params[i]) && start != -1) {
            std::string_view hex_word = all_params;
            params.emplace_back(hex_to_word(hex_word.substr(start, i - start)));
            start = -1;
          }
        }
        handle_write(params);
      }
      else if (std::regex_search(command, match, print_hex)) {
        std::string const& address = match[2].str();
        std::string const& num_words = match[3].str();
        handle_hdump(hex_to_word(address), std::strtoul(num_words.c_str(), nullptr, 10));
      }
      else if (std::regex_search(command, match, print_instructions)) {
        std::string const& address = match[2].str();
        std::string const& num_instructions = match[3].str();
        handle_idump(hex_to_word(address), std::strtoul(num_instructions.c_str(), nullptr, 10));
      }
      else if (std::regex_search(command, match, print_float)) {
        std::string const& address = match[2].str();
        std::string const& num_floats = match[3].str();
        handle_fdump(hex_to_word(address), std::strtoul(num_floats.c_str(), nullptr, 10));
      }
      else
      {
        std::cout << "Invalid command \"" << command << "\"" << std::endl;
      }
    }
  }

  void initialize_cli() {
    if (!initialized) {
      initialized = true;
    }
    else { return; }
    cli_thread_handle = std::make_unique<std::thread>(cli_thread);
    cli_thread_handle->detach();
  }
}

#else

namespace prime {
  void initialize_cli() {}
}

#endif
