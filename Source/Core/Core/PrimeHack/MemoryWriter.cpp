#include "Core/PrimeHack/MemoryWriter.h"

#ifdef _WIN32

#include <Windows.h>

#include <cctype>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <thread>

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
      return c - 'a';
    }
    else if (c >= 'A' && c <= 'F') {
      return c - 'A';
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

  }

  void handle_hdump(uint32_t address, int num_words) {

  }

  void handle_idump(uint32_t address, int num_instructions) {

  }

  void handle_fdump(uint32_t address, int num_floats) {

  }

  void cli_thread() {
    AllocConsole();
    freopen("CONIN$", "w", stdin);

    const std::regex write_memory("^write( +(0x)?([a-fA-F0-9]{8}))+ *$");
    const std::regex print_hex("^hdump +(0x)?([a-fA-F0-9]{8}) +(\d+) *$");
    const std::regex print_instructions("^idump +(0x)?([a-fA-F0-9]{8}) +(\d+) *$");
    const std::regex print_float("^fdump +(0x)?([a-fA-F0-9]{8}) +(\d+) *$");
    while (true) {
      std::string command;
      std::smatch match;
      std::getline(std::cin, command);

      if (std::regex_search(command, match, write_memory)) {
        // params[0] = address, params[1+] = writes
        std::vector<uint32_t> params;

        std::string const& all_params = match[0].str();
        int start = -1;
        for (int i = 0; i < all_params.length(); i++) {
          if (!std::isspace(all_params[i])) {
            start = i;
          }
          else if (start != -1) {
            std::string_view hex_word = all_params;
            params.emplace_back(hex_to_word(hex_word.substr(i)));
            start = -1;
          }
        }
        handle_write(params);
      }
      else if (std::regex_search(command, match, print_hex)) {
        std::string const& address = match[1].str();
        std::string const& num_words = match[2].str();
        handle_hdump(hex_to_word(address), std::strtol(num_words.c_str(), nullptr, 10));
      }
      else if (std::regex_search(command, match, print_instructions)) {
        std::string const& address = match[1].str();
        std::string const& num_instructions = match[2].str();
        handle_idump(hex_to_word(address), std::strtol(num_instructions.c_str(), nullptr, 10));
      }
      else if (std::regex_search(command, match, print_float)) {
        std::string const& address = match[1].str();
        std::string const& num_floats = match[2].str();
        handle_fdump(hex_to_word(address), std::strtol(num_floats.c_str(), nullptr, 10));
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
