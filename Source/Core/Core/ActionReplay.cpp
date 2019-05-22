// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

// -----------------------------------------------------------------------------------------
// Partial Action Replay code system implementation.
// Will never be able to support some AR codes - specifically those that patch the running
// Action Replay engine itself - yes they do exist!!!
// Action Replay actually is a small virtual machine with a limited number of commands.
// It probably is Turing complete - but what does that matter when AR codes can write
// actual PowerPC code...
// -----------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------------------
// Code Types:
// (Unconditional) Normal Codes (0): this one has subtypes inside
// (Conditional) Normal Codes (1 - 7): these just compare values and set the line skip info
// Zero Codes: any code with no address.  These codes are used to do special operations like memory
// copy, etc
// -------------------------------------------------------------------------------------------------------------

#include "Core/ActionReplay.h"

#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <iterator>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Common/CommonTypes.h"
#include "Common/IniFile.h"
#include "Common/Logging/Log.h"
#include "Common/MsgHandler.h"
#include "Common/StringUtil.h"

#include "Core/ARDecrypt.h"
#include "Core/ConfigManager.h"
#include "Core/PowerPC/PowerPC.h"

#include "HackConfig.h"
#include "InputCommon/GenericMouse.h"
#include "VideoCommon/RenderBase.h"

namespace ActionReplay
{
#define clamp(min, max, v) ((v) > (max) ? (max) : ((v) < (min) ? (min) : (v)))
//  turning rate horizontal for Prime 1 is approximately this (in rad/sec)
#define TURNRATE_RATIO 0.00498665500569808449206349206349f
typedef union
{
  u32 i;
  float f;
} rawfloat;
enum
{
  // Zero Code Types
  ZCODE_END = 0x00,
  ZCODE_NORM = 0x02,
  ZCODE_ROW = 0x03,
  ZCODE_04 = 0x04,

  // Conditional Codes
  CONDTIONAL_EQUAL = 0x01,
  CONDTIONAL_NOT_EQUAL = 0x02,
  CONDTIONAL_LESS_THAN_SIGNED = 0x03,
  CONDTIONAL_GREATER_THAN_SIGNED = 0x04,
  CONDTIONAL_LESS_THAN_UNSIGNED = 0x05,
  CONDTIONAL_GREATER_THAN_UNSIGNED = 0x06,
  CONDTIONAL_AND = 0x07,  // bitwise AND

  // Conditional Line Counts
  CONDTIONAL_ONE_LINE = 0x00,
  CONDTIONAL_TWO_LINES = 0x01,
  CONDTIONAL_ALL_LINES_UNTIL = 0x02,
  CONDTIONAL_ALL_LINES = 0x03,

  // Data Types
  DATATYPE_8BIT = 0x00,
  DATATYPE_16BIT = 0x01,
  DATATYPE_32BIT = 0x02,
  DATATYPE_32BIT_FLOAT = 0x03,

  // Normal Code 0 Subtypes
  SUB_RAM_WRITE = 0x00,
  SUB_WRITE_POINTER = 0x01,
  SUB_ADD_CODE = 0x02,
  SUB_MASTER_CODE = 0x03,
};

// General lock. Protects codes list and internal log.
static std::mutex s_lock;
static std::vector<ARCode> s_active_codes;
static std::vector<std::string> s_internal_log;
static std::atomic<bool> s_use_internal_log{false};
static int active_game = 1;
// pointer to the code currently being run, (used by log messages that include the code name)
static const ARCode* s_current_code = nullptr;
static bool s_disable_logging = false;
static bool s_window_focused = false;
static bool s_cursor_locked = false;

struct ARAddr
{
  union
  {
    u32 address;
    struct
    {
      u32 gcaddr : 25;
      u32 size : 2;
      u32 type : 3;
      u32 subtype : 2;
    };
  };

  ARAddr(const u32 addr) : address(addr) {}
  u32 GCAddress() const { return gcaddr | 0x80000000; }
  operator u32() const { return address; }
};

// ----------------------
// AR Remote Functions
void ApplyCodes(const std::vector<ARCode>& codes)
{
  if (!SConfig::GetInstance().bEnableCheats)
    return;

  std::lock_guard<std::mutex> guard(s_lock);
  s_disable_logging = false;
  s_active_codes.clear();
  std::copy_if(codes.begin(), codes.end(), std::back_inserter(s_active_codes),
               [](const ARCode& code) { return code.active; });
  s_active_codes.shrink_to_fit();
}

void AddCode(ARCode code)
{
  if (!SConfig::GetInstance().bEnableCheats)
    return;

  if (code.active)
  {
    std::lock_guard<std::mutex> guard(s_lock);
    s_disable_logging = false;
    s_active_codes.emplace_back(std::move(code));
  }
}

void LoadAndApplyCodes(const IniFile& global_ini, const IniFile& local_ini)
{
  ApplyCodes(LoadCodes(global_ini, local_ini));
}

// Parses the Action Replay section of a game ini file.
std::vector<ARCode> LoadCodes(const IniFile& global_ini, const IniFile& local_ini)
{
  std::vector<ARCode> codes;

  std::unordered_set<std::string> enabled_names;
  {
    std::vector<std::string> enabled_lines;
    local_ini.GetLines("ActionReplay_Enabled", &enabled_lines);
    for (const std::string& line : enabled_lines)
    {
      if (line.size() != 0 && line[0] == '$')
      {
        std::string name = line.substr(1, line.size() - 1);
        enabled_names.insert(name);
      }
    }
  }

  const IniFile* inis[2] = {&global_ini, &local_ini};
  for (const IniFile* ini : inis)
  {
    std::vector<std::string> lines;
    std::vector<std::string> encrypted_lines;
    ARCode current_code;

    ini->GetLines("ActionReplay", &lines);

    for (const std::string& line : lines)
    {
      if (line.empty())
      {
        continue;
      }

      // Check if the line is a name of the code
      if (line[0] == '$')
      {
        if (current_code.ops.size())
        {
          codes.push_back(current_code);
          current_code.ops.clear();
        }
        if (encrypted_lines.size())
        {
          DecryptARCode(encrypted_lines, &current_code.ops);
          codes.push_back(current_code);
          current_code.ops.clear();
          encrypted_lines.clear();
        }

        current_code.name = line.substr(1, line.size() - 1);
        current_code.active = enabled_names.find(current_code.name) != enabled_names.end();
        current_code.user_defined = (ini == &local_ini);
      }
      else
      {
        std::vector<std::string> pieces = SplitString(line, ' ');

        // Check if the AR code is decrypted
        if (pieces.size() == 2 && pieces[0].size() == 8 && pieces[1].size() == 8)
        {
          AREntry op;
          bool success_addr = TryParse(std::string("0x") + pieces[0], &op.cmd_addr);
          bool success_val = TryParse(std::string("0x") + pieces[1], &op.value);

          if (success_addr && success_val)
          {
            current_code.ops.push_back(op);
          }
          else
          {
            PanicAlertT("Action Replay Error: invalid AR code line: %s", line.c_str());

            if (!success_addr)
              PanicAlertT("The address is invalid");

            if (!success_val)
              PanicAlertT("The value is invalid");
          }
        }
        else
        {
          pieces = SplitString(line, '-');
          if (pieces.size() == 3 && pieces[0].size() == 4 && pieces[1].size() == 4 &&
              pieces[2].size() == 5)
          {
            // Encrypted AR code
            // Decryption is done in "blocks", so we must push blocks into a vector,
            // then send to decrypt when a new block is encountered, or if it's the last block.
            encrypted_lines.emplace_back(pieces[0] + pieces[1] + pieces[2]);
          }
        }
      }
    }

    // Handle the last code correctly.
    if (current_code.ops.size())
    {
      codes.push_back(current_code);
    }
    if (encrypted_lines.size())
    {
      DecryptARCode(encrypted_lines, &current_code.ops);
      codes.push_back(current_code);
    }
  }

  return codes;
}

void SaveCodes(IniFile* local_ini, const std::vector<ARCode>& codes)
{
  std::vector<std::string> lines;
  std::vector<std::string> enabled_lines;
  for (const ActionReplay::ARCode& code : codes)
  {
    if (code.active)
      enabled_lines.emplace_back("$" + code.name);

    if (code.user_defined)
    {
      lines.emplace_back("$" + code.name);
      for (const ActionReplay::AREntry& op : code.ops)
      {
        lines.emplace_back(StringFromFormat("%08X %08X", op.cmd_addr, op.value));
      }
    }
  }
  local_ini->SetLines("ActionReplay_Enabled", enabled_lines);
  local_ini->SetLines("ActionReplay", lines);
}

static void LogInfo(const char* format, ...)
{
  if (s_disable_logging)
    return;
  bool use_internal_log = s_use_internal_log.load(std::memory_order_relaxed);
  if (MAX_LOGLEVEL < LogTypes::LINFO && !use_internal_log)
    return;

  va_list args;
  va_start(args, format);
  std::string text = StringFromFormatV(format, args);
  va_end(args);
  INFO_LOG(ACTIONREPLAY, "%s", text.c_str());

  if (use_internal_log)
  {
    text += '\n';
    s_internal_log.emplace_back(std::move(text));
  }
}

void EnableSelfLogging(bool enable)
{
  s_use_internal_log.store(enable, std::memory_order_relaxed);
}

std::vector<std::string> GetSelfLog()
{
  std::lock_guard<std::mutex> guard(s_lock);
  return s_internal_log;
}

void ClearSelfLog()
{
  std::lock_guard<std::mutex> guard(s_lock);
  s_internal_log.clear();
}

bool IsSelfLogging()
{
  return s_use_internal_log.load(std::memory_order_relaxed);
}

// ----------------------
// Code Functions
static bool Subtype_RamWriteAndFill(const ARAddr& addr, const u32 data)
{
  const u32 new_addr = addr.GCAddress();

  LogInfo("Hardware Address: %08x", new_addr);
  LogInfo("Size: %08x", addr.size);

  switch (addr.size)
  {
  case DATATYPE_8BIT:
  {
    LogInfo("8-bit Write");
    LogInfo("--------");
    u32 repeat = data >> 8;
    for (u32 i = 0; i <= repeat; ++i)
    {
      PowerPC::HostWrite_U8(data & 0xFF, new_addr + i);
      LogInfo("Wrote %08x to address %08x", data & 0xFF, new_addr + i);
    }
    LogInfo("--------");
    break;
  }

  case DATATYPE_16BIT:
  {
    LogInfo("16-bit Write");
    LogInfo("--------");
    u32 repeat = data >> 16;
    for (u32 i = 0; i <= repeat; ++i)
    {
      PowerPC::HostWrite_U16(data & 0xFFFF, new_addr + i * 2);
      LogInfo("Wrote %08x to address %08x", data & 0xFFFF, new_addr + i * 2);
    }
    LogInfo("--------");
    break;
  }

  case DATATYPE_32BIT_FLOAT:
  case DATATYPE_32BIT:  // Dword write
    LogInfo("32-bit Write");
    LogInfo("--------");
    PowerPC::HostWrite_U32(data, new_addr);
    LogInfo("Wrote %08x to address %08x", data, new_addr);
    LogInfo("--------");
    break;

  default:
    LogInfo("Bad Size");
    PanicAlertT("Action Replay Error: Invalid size "
                "(%08x : address = %08x) in Ram Write And Fill (%s)",
                addr.size, addr.gcaddr, s_current_code->name.c_str());
    return false;
  }

  return true;
}

static bool Subtype_WriteToPointer(const ARAddr& addr, const u32 data)
{
  const u32 new_addr = addr.GCAddress();
  const u32 ptr = PowerPC::HostRead_U32(new_addr);

  LogInfo("Hardware Address: %08x", new_addr);
  LogInfo("Size: %08x", addr.size);

  switch (addr.size)
  {
  case DATATYPE_8BIT:
  {
    LogInfo("Write 8-bit to pointer");
    LogInfo("--------");
    const u8 thebyte = data & 0xFF;
    const u32 offset = data >> 8;
    LogInfo("Pointer: %08x", ptr);
    LogInfo("Byte: %08x", thebyte);
    LogInfo("Offset: %08x", offset);
    PowerPC::HostWrite_U8(thebyte, ptr + offset);
    LogInfo("Wrote %08x to address %08x", thebyte, ptr + offset);
    LogInfo("--------");
    break;
  }

  case DATATYPE_16BIT:
  {
    LogInfo("Write 16-bit to pointer");
    LogInfo("--------");
    const u16 theshort = data & 0xFFFF;
    const u32 offset = (data >> 16) << 1;
    LogInfo("Pointer: %08x", ptr);
    LogInfo("Byte: %08x", theshort);
    LogInfo("Offset: %08x", offset);
    PowerPC::HostWrite_U16(theshort, ptr + offset);
    LogInfo("Wrote %08x to address %08x", theshort, ptr + offset);
    LogInfo("--------");
    break;
  }

  case DATATYPE_32BIT_FLOAT:
  case DATATYPE_32BIT:
    LogInfo("Write 32-bit to pointer");
    LogInfo("--------");
    PowerPC::HostWrite_U32(data, ptr);
    LogInfo("Wrote %08x to address %08x", data, ptr);
    LogInfo("--------");
    break;

  default:
    LogInfo("Bad Size");
    PanicAlertT("Action Replay Error: Invalid size "
                "(%08x : address = %08x) in Write To Pointer (%s)",
                addr.size, addr.gcaddr, s_current_code->name.c_str());
    return false;
  }
  return true;
}

static bool Subtype_AddCode(const ARAddr& addr, const u32 data)
{
  // Used to increment/decrement a value in memory
  const u32 new_addr = addr.GCAddress();

  LogInfo("Hardware Address: %08x", new_addr);
  LogInfo("Size: %08x", addr.size);

  switch (addr.size)
  {
  case DATATYPE_8BIT:
    LogInfo("8-bit Add");
    LogInfo("--------");
    PowerPC::HostWrite_U8(PowerPC::HostRead_U8(new_addr) + data, new_addr);
    LogInfo("Wrote %02x to address %08x", PowerPC::HostRead_U8(new_addr), new_addr);
    LogInfo("--------");
    break;

  case DATATYPE_16BIT:
    LogInfo("16-bit Add");
    LogInfo("--------");
    PowerPC::HostWrite_U16(PowerPC::HostRead_U16(new_addr) + data, new_addr);
    LogInfo("Wrote %04x to address %08x", PowerPC::HostRead_U16(new_addr), new_addr);
    LogInfo("--------");
    break;

  case DATATYPE_32BIT:
    LogInfo("32-bit Add");
    LogInfo("--------");
    PowerPC::HostWrite_U32(PowerPC::HostRead_U32(new_addr) + data, new_addr);
    LogInfo("Wrote %08x to address %08x", PowerPC::HostRead_U32(new_addr), new_addr);
    LogInfo("--------");
    break;

  case DATATYPE_32BIT_FLOAT:
  {
    LogInfo("32-bit floating Add");
    LogInfo("--------");

    const u32 read = PowerPC::HostRead_U32(new_addr);
    const float read_float = reinterpret_cast<const float&>(read);
    // data contains an (unsigned?) integer value
    const float fread = read_float + static_cast<float>(data);
    const u32 newval = reinterpret_cast<const u32&>(fread);
    PowerPC::HostWrite_U32(newval, new_addr);
    LogInfo("Old Value %08x", read);
    LogInfo("Increment %08x", data);
    LogInfo("New value %08x", newval);
    LogInfo("--------");
    break;
  }

  default:
    LogInfo("Bad Size");
    PanicAlertT("Action Replay Error: Invalid size "
                "(%08x : address = %08x) in Add Code (%s)",
                addr.size, addr.gcaddr, s_current_code->name.c_str());
    return false;
  }
  return true;
}

static bool Subtype_MasterCodeAndWriteToCCXXXXXX(const ARAddr& addr, const u32 data)
{
  // code not yet implemented - TODO
  // u32 new_addr = (addr & 0x01FFFFFF) | 0x80000000;
  // u8  mcode_type = (data & 0xFF0000) >> 16;
  // u8  mcode_count = (data & 0xFF00) >> 8;
  // u8  mcode_number = data & 0xFF;
  PanicAlertT("Action Replay Error: Master Code and Write To CCXXXXXX not implemented (%s)\n"
              "Master codes are not needed. Do not use master codes.",
              s_current_code->name.c_str());
  return false;
}

// This needs more testing
static bool ZeroCode_FillAndSlide(const u32 val_last, const ARAddr& addr, const u32 data)
{
  const u32 new_addr = ARAddr(val_last).GCAddress();
  const u8 size = ARAddr(val_last).size;

  const s16 addr_incr = static_cast<s16>(data & 0xFFFF);
  const s8 val_incr = static_cast<s8>(data >> 24);
  const u8 write_num = static_cast<u8>((data & 0xFF0000) >> 16);

  u32 val = addr;
  u32 curr_addr = new_addr;

  LogInfo("Current Hardware Address: %08x", new_addr);
  LogInfo("Size: %08x", addr.size);
  LogInfo("Write Num: %08x", write_num);
  LogInfo("Address Increment: %i", addr_incr);
  LogInfo("Value Increment: %i", val_incr);

  switch (size)
  {
  case DATATYPE_8BIT:
    LogInfo("8-bit Write");
    LogInfo("--------");
    for (int i = 0; i < write_num; ++i)
    {
      PowerPC::HostWrite_U8(val & 0xFF, curr_addr);
      curr_addr += addr_incr;
      val += val_incr;
      LogInfo("Write %08x to address %08x", val & 0xFF, curr_addr);

      LogInfo("Value Update: %08x", val);
      LogInfo("Current Hardware Address Update: %08x", curr_addr);
    }
    LogInfo("--------");
    break;

  case DATATYPE_16BIT:
    LogInfo("16-bit Write");
    LogInfo("--------");
    for (int i = 0; i < write_num; ++i)
    {
      PowerPC::HostWrite_U16(val & 0xFFFF, curr_addr);
      LogInfo("Write %08x to address %08x", val & 0xFFFF, curr_addr);
      curr_addr += addr_incr * 2;
      val += val_incr;
      LogInfo("Value Update: %08x", val);
      LogInfo("Current Hardware Address Update: %08x", curr_addr);
    }
    LogInfo("--------");
    break;

  case DATATYPE_32BIT:
    LogInfo("32-bit Write");
    LogInfo("--------");
    for (int i = 0; i < write_num; ++i)
    {
      PowerPC::HostWrite_U32(val, curr_addr);
      LogInfo("Write %08x to address %08x", val, curr_addr);
      curr_addr += addr_incr * 4;
      val += val_incr;
      LogInfo("Value Update: %08x", val);
      LogInfo("Current Hardware Address Update: %08x", curr_addr);
    }
    LogInfo("--------");
    break;

  default:
    LogInfo("Bad Size");
    PanicAlertT("Action Replay Error: Invalid size (%08x : address = %08x) in Fill and Slide (%s)",
                size, new_addr, s_current_code->name.c_str());
    return false;
  }
  return true;
}

// kenobi's "memory copy" Z-code. Requires an additional master code
// on a real AR device. Documented here:
// https://github.com/dolphin-emu/dolphin/wiki/GameCube-Action-Replay-Code-Types#type-z4-size-3--memory-copy
static bool ZeroCode_MemoryCopy(const u32 val_last, const ARAddr& addr, const u32 data)
{
  const u32 addr_dest = val_last & ~0x06000000;
  const u32 addr_src = addr.GCAddress();

  const u8 num_bytes = data & 0x7FFF;

  LogInfo("Dest Address: %08x", addr_dest);
  LogInfo("Src Address: %08x", addr_src);
  LogInfo("Size: %08x", num_bytes);

  if ((data & 0xFF0000) == 0)
  {
    if ((data >> 24) != 0x0)
    {  // Memory Copy With Pointers Support
      LogInfo("Memory Copy With Pointers Support");
      LogInfo("--------");
      const u32 ptr_dest = PowerPC::HostRead_U32(addr_dest);
      LogInfo("Resolved Dest Address to: %08x", ptr_dest);
      const u32 ptr_src = PowerPC::HostRead_U32(addr_src);
      LogInfo("Resolved Src Address to: %08x", ptr_src);
      for (int i = 0; i < num_bytes; ++i)
      {
        PowerPC::HostWrite_U8(PowerPC::HostRead_U8(ptr_src + i), ptr_dest + i);
        LogInfo("Wrote %08x to address %08x", PowerPC::HostRead_U8(ptr_src + i), ptr_dest + i);
      }
      LogInfo("--------");
    }
    else
    {  // Memory Copy Without Pointer Support
      LogInfo("Memory Copy Without Pointers Support");
      LogInfo("--------");
      for (int i = 0; i < num_bytes; ++i)
      {
        PowerPC::HostWrite_U8(PowerPC::HostRead_U8(addr_src + i), addr_dest + i);
        LogInfo("Wrote %08x to address %08x", PowerPC::HostRead_U8(addr_src + i), addr_dest + i);
      }
      LogInfo("--------");
      return true;
    }
  }
  else
  {
    LogInfo("Bad Value");
    PanicAlertT("Action Replay Error: Invalid value (%08x) in Memory Copy (%s)", (data & ~0x7FFF),
                s_current_code->name.c_str());
    return false;
  }
  return true;
}

static bool NormalCode(const ARAddr& addr, const u32 data)
{
  switch (addr.subtype)
  {
  case SUB_RAM_WRITE:  // Ram write (and fill)
    LogInfo("Doing Ram Write And Fill");
    if (!Subtype_RamWriteAndFill(addr, data))
      return false;
    break;

  case SUB_WRITE_POINTER:  // Write to pointer
    LogInfo("Doing Write To Pointer");
    if (!Subtype_WriteToPointer(addr, data))
      return false;
    break;

  case SUB_ADD_CODE:  // Increment Value
    LogInfo("Doing Add Code");
    if (!Subtype_AddCode(addr, data))
      return false;
    break;

  case SUB_MASTER_CODE:  // Master Code & Write to CCXXXXXX
    LogInfo("Doing Master Code And Write to CCXXXXXX (ncode not supported)");
    if (!Subtype_MasterCodeAndWriteToCCXXXXXX(addr, data))
      return false;
    break;

  default:
    LogInfo("Bad Subtype");
    PanicAlertT("Action Replay: Normal Code 0: Invalid Subtype %08x (%s)", addr.subtype,
                s_current_code->name.c_str());
    return false;
  }

  return true;
}

static bool CompareValues(const u32 val1, const u32 val2, const int type)
{
  switch (type)
  {
  case CONDTIONAL_EQUAL:
    LogInfo("Type 1: If Equal");
    return val1 == val2;

  case CONDTIONAL_NOT_EQUAL:
    LogInfo("Type 2: If Not Equal");
    return val1 != val2;

  case CONDTIONAL_LESS_THAN_SIGNED:
    LogInfo("Type 3: If Less Than (Signed)");
    return static_cast<s32>(val1) < static_cast<s32>(val2);

  case CONDTIONAL_GREATER_THAN_SIGNED:
    LogInfo("Type 4: If Greater Than (Signed)");
    return static_cast<s32>(val1) > static_cast<s32>(val2);

  case CONDTIONAL_LESS_THAN_UNSIGNED:
    LogInfo("Type 5: If Less Than (Unsigned)");
    return val1 < val2;

  case CONDTIONAL_GREATER_THAN_UNSIGNED:
    LogInfo("Type 6: If Greater Than (Unsigned)");
    return val1 > val2;

  case CONDTIONAL_AND:
    LogInfo("Type 7: If And");
    return !!(val1 & val2);  // bitwise AND

  default:
    LogInfo("Unknown Compare type");
    PanicAlertT("Action Replay: Invalid Normal Code Type %08x (%s)", type,
                s_current_code->name.c_str());
    return false;
  }
}

static bool ConditionalCode(const ARAddr& addr, const u32 data, int* const pSkipCount)
{
  const u32 new_addr = addr.GCAddress();

  LogInfo("Size: %08x", addr.size);
  LogInfo("Hardware Address: %08x", new_addr);

  bool result = true;

  switch (addr.size)
  {
  case DATATYPE_8BIT:
    result = CompareValues(PowerPC::HostRead_U8(new_addr), (data & 0xFF), addr.type);
    break;

  case DATATYPE_16BIT:
    result = CompareValues(PowerPC::HostRead_U16(new_addr), (data & 0xFFFF), addr.type);
    break;

  case DATATYPE_32BIT_FLOAT:
  case DATATYPE_32BIT:
    result = CompareValues(PowerPC::HostRead_U32(new_addr), data, addr.type);
    break;

  default:
    LogInfo("Bad Size");
    PanicAlertT("Action Replay: Conditional Code: Invalid Size %08x (%s)", addr.size,
                s_current_code->name.c_str());
    return false;
  }

  // if the comparison failed we need to skip some lines
  if (false == result)
  {
    switch (addr.subtype)
    {
    case CONDTIONAL_ONE_LINE:
    case CONDTIONAL_TWO_LINES:
      *pSkipCount = addr.subtype + 1;  // Skip 1 or 2 lines
      break;

      // Skip all lines,
      // Skip lines until a "00000000 40000000" line is reached
    case CONDTIONAL_ALL_LINES:
    case CONDTIONAL_ALL_LINES_UNTIL:
      *pSkipCount = -static_cast<int>(addr.subtype);
      break;

    default:
      LogInfo("Bad Subtype");
      PanicAlertT("Action Replay: Normal Code %i: Invalid subtype %08x (%s)", 1, addr.subtype,
                  s_current_code->name.c_str());
      return false;
    }
  }

  return true;
}

// NOTE: Lock needed to give mutual exclusion to s_current_code and LogInfo
static bool RunCodeLocked(const ARCode& arcode)
{
  // The mechanism is different than what the real AR uses, so there may be compatibility problems.

  bool do_fill_and_slide = false;
  bool do_memory_copy = false;

  // used for conditional codes
  int skip_count = 0;

  u32 val_last = 0;

  s_current_code = &arcode;

  LogInfo("Code Name: %s", arcode.name.c_str());
  LogInfo("Number of codes: %zu", arcode.ops.size());

  for (const AREntry& entry : arcode.ops)
  {
    const ARAddr addr(entry.cmd_addr);
    const u32 data = entry.value;

    // after a conditional code, skip lines if needed
    if (skip_count)
    {
      if (skip_count > 0)  // skip x lines
      {
        LogInfo("Line skipped");
        --skip_count;
      }
      else if (-CONDTIONAL_ALL_LINES == skip_count)
      {
        // skip all lines
        LogInfo("All Lines skipped");
        return true;  // don't need to iterate through the rest of the ops
      }
      else if (-CONDTIONAL_ALL_LINES_UNTIL == skip_count)
      {
        // skip until a "00000000 40000000" line is reached
        LogInfo("Line skipped");
        if (addr == 0 && 0x40000000 == data)  // check for an endif line
          skip_count = 0;
      }

      continue;
    }

    LogInfo("--- Running Code: %08x %08x ---", addr.address, data);
    // LogInfo("Command: %08x", cmd);

    // Do Fill & Slide
    if (do_fill_and_slide)
    {
      do_fill_and_slide = false;
      LogInfo("Doing Fill And Slide");
      if (false == ZeroCode_FillAndSlide(val_last, addr, data))
        return false;
      continue;
    }

    // Memory Copy
    if (do_memory_copy)
    {
      do_memory_copy = false;
      LogInfo("Doing Memory Copy");
      if (false == ZeroCode_MemoryCopy(val_last, addr, data))
        return false;
      continue;
    }

    // ActionReplay program self modification codes
    if (addr >= 0x00002000 && addr < 0x00003000)
    {
      LogInfo(
          "This action replay simulator does not support codes that modify Action Replay itself.");
      PanicAlertT(
          "This action replay simulator does not support codes that modify Action Replay itself.");
      return false;
    }

    // skip these weird init lines
    // TODO: Where are the "weird init lines"?
    // if (iter == code.ops.begin() && cmd == 1)
    // continue;

    // Zero codes
    if (0x0 == addr)  // Check if the code is a zero code
    {
      const u8 zcode = data >> 29;

      LogInfo("Doing Zero Code %08x", zcode);

      switch (zcode)
      {
      case ZCODE_END:  // END OF CODES
        LogInfo("ZCode: End Of Codes");
        return true;

        // TODO: the "00000000 40000000"(end if) codes fall into this case, I don't think that is
        // correct
      case ZCODE_NORM:  // Normal execution of codes
                        // Todo: Set register 1BB4 to 0
        LogInfo("ZCode: Normal execution of codes, set register 1BB4 to 0 (zcode not supported)");
        break;

      case ZCODE_ROW:  // Executes all codes in the same row
                       // Todo: Set register 1BB4 to 1
        LogInfo("ZCode: Executes all codes in the same row, Set register 1BB4 to 1 (zcode not "
                "supported)");
        PanicAlertT("Zero 3 code not supported");
        return false;

      case ZCODE_04:  // Fill & Slide or Memory Copy
        if (0x3 == ((data >> 25) & 0x03))
        {
          LogInfo("ZCode: Memory Copy");
          do_memory_copy = true;
          val_last = data;
        }
        else
        {
          LogInfo("ZCode: Fill And Slide");
          do_fill_and_slide = true;
          val_last = data;
        }
        break;

      default:
        LogInfo("ZCode: Unknown");
        PanicAlertT("Zero code unknown to Dolphin: %08x", zcode);
        return false;
      }

      // done handling zero codes
      continue;
    }

    // Normal codes
    LogInfo("Doing Normal Code %08x", addr.type);
    LogInfo("Subtype: %08x", addr.subtype);

    switch (addr.type)
    {
    case 0x00:
      if (false == NormalCode(addr, data))
        return false;
      break;

    default:
      LogInfo("This Normal Code is a Conditional Code");
      if (false == ConditionalCode(addr, data, &skip_count))
        return false;
      break;
    }
  }

  return true;
}

bool mem_check(u32 address)
{
  return (address >= 0x80000000) && (address < 0x81800000);
}

float sgn(float val)
{
  return static_cast<float>((val > 0.f) - (val < 0.f));
}

void OnMouseClick(wxMouseEvent& event)
{
  s_window_focused = true;
}

float getAspectRatio()
{
  const unsigned int scale = g_renderer->GetEFBScale();
  float sW = scale * EFB_WIDTH;
  float sH = scale * EFB_HEIGHT;
  return sW / sH;
}

static float cursor_xPosition = 0;
static float cursor_yPosition = 0;

void handleCursor(u32 x_address, u32 y_address, float rbound, float bbound)
{
  int dx = prime::g_mouse_input->GetDeltaHorizontalAxis(),
    dy = prime::g_mouse_input->GetDeltaVerticalAxis();

  float aspect_ratio = getAspectRatio();
  if (isnan(aspect_ratio))
    return;

  float cursor_sensitivity_conv = prime::GetCursorSensitivity() / 50.f;

  cursor_xPosition += ((float)dx * cursor_sensitivity_conv / 200.f);
  cursor_yPosition += ((float)dy * aspect_ratio * cursor_sensitivity_conv / 200.f);

  cursor_xPosition = clamp(-1, rbound, cursor_xPosition);
  cursor_yPosition = clamp(-1, bbound, cursor_yPosition);

  u32 xp, yp;

  memcpy(&xp, &cursor_xPosition, sizeof(u32));
  memcpy(&yp, &cursor_yPosition, sizeof(u32));

  PowerPC::HostWrite_U32(xp, x_address);
  PowerPC::HostWrite_U32(yp, y_address);
}

void primeMenu_NTSC()
{
  handleCursor(0x80913c9c, 0x80913d5c, 0.95f, 0.90f);
}

void primeMenu_PAL()
{
  u32 cursorBaseAddr = PowerPC::HostRead_U32(0x80621ffc);
  handleCursor(cursorBaseAddr + 0xdc, cursorBaseAddr + 0x19c, 0.95f, 0.90f);
}

std::tuple<int, int> getVisorSwitch(std::array<std::tuple<int, int>, 4> const& visors)
{
  static bool pressing_button = false;
  if (prime::CheckVisorCtl(0))
  {
    if (!pressing_button)
    {
      pressing_button = true;
      return visors[0];
    }
  }
  else if (prime::CheckVisorCtl(1))
  {
    if (!pressing_button)
    {
      pressing_button = true;
      return visors[1];
    }
  }
  else if (prime::CheckVisorCtl(2))
  {
    if (!pressing_button)
    {
      pressing_button = true;
      return visors[2];
    }
  }
  else if (prime::CheckVisorCtl(3))
  {
    if (!pressing_button)
    {
      pressing_button = true;
      return visors[3];
    }
  }
  else
  {
    pressing_button = false;
  }
  return std::make_tuple(-1, 0);
}

int getBeamSwitch(std::array<int, 4> const& beams)
{
  static bool pressing_button = false;
  if (prime::CheckBeamCtl(0))
  {
    if (!pressing_button)
    {
      pressing_button = true;
      return beams[0];
    }
  }
  else if (prime::CheckBeamCtl(1))
  {
    if (!pressing_button)
    {
      pressing_button = true;
      return beams[1];
    }
  }
  else if (prime::CheckBeamCtl(2))
  {
    if (!pressing_button)
    {
      pressing_button = true;
      return beams[2];
    }
  }
  else if (prime::CheckBeamCtl(3))
  {
    if (!pressing_button)
    {
      pressing_button = true;
      return beams[3];
    }
  }
  else
  {
    pressing_button = false;
  }
  return -1;
}

/*
 * Going to comment this up for future reference
 * Prime one beam IDs: 0 = power, 1 = ice, 2 = wave, 3 = plasma
 * Prime one visor IDs: 0 = combat, 1 = xray, 2 = scan, 3 = thermal
 * Prime two beam IDs: 0 = power, 1 = dark, 2 = light, 3 = annihilator
 * Prime two visor IDs: 0 = combat, 1 = echo, 2 = scan, 3 = dark
 * ADDITIONAL INFO: Equipment have-status offsets:
 * Beams can be ignored (for now) as the existing code handles that for us
 * Prime one visor offsets: combat = 0x11, scan = 0x05, thermal = 0x09, xray = 0x0d
 * Prime two visor offsets: combat = 0x08, scan = 0x09, dark = 0x0d, echo = 0x0b
 */
static std::array<int, 4> prime_one_beams = {0, 2, 1, 3};
static std::array<int, 4> prime_two_beams = {0, 1, 2, 3};
// it can not be explained why combat->xray->scan->thermal is the ordering...
static std::array<std::tuple<int, int>, 4> prime_one_visors = {
    std::make_tuple<int, int>(0, 0x11), std::make_tuple<int, int>(2, 0x05),
    std::make_tuple<int, int>(3, 0x09), std::make_tuple<int, int>(1, 0x0d) };
static std::array<std::tuple<int, int>, 4> prime_two_visors = {
    std::make_tuple<int, int>(0, 0x08), std::make_tuple<int, int>(2, 0x09),
    std::make_tuple<int, int>(3, 0x0d), std::make_tuple<int, int>(1, 0x0b) };
static std::array<std::tuple<int, int>, 4> prime_three_visors = {
    std::make_tuple<int, int>(0, 0x0b), std::make_tuple<int, int>(1, 0x0c),
    std::make_tuple<int, int>(2, 0x0d), std::make_tuple<int, int>(3, 0x0e) };

//*****************************************************************************************
// Metroid Prime 1
//*****************************************************************************************
void primeOne_NTSC()
{
  // Flag which indicates lock-on
  if (PowerPC::HostRead_U8(0x804C00B3))
  {
    PowerPC::HostWrite_U32(0, 0x804d3d38);
    return;
  }

  // for vertical angle control, we need to send the actual direction to look
  // i believe the angle is measured in radians, clamped ~[-1.22, 1.22]
  static float yAngle = 0;

  int dx = prime::g_mouse_input->GetDeltaHorizontalAxis(),
      dy = prime::g_mouse_input->GetDeltaVerticalAxis();

  float vSensitivity = (prime::GetSensitivity() * TURNRATE_RATIO) / (60.0f);

  float dfx = dx * -prime::GetSensitivity();
  yAngle += ((float)dy * -vSensitivity);
  yAngle = clamp(-1.22f, 1.22f, yAngle);

  u32 horizontalSpeed, verticalAngle;
  memcpy(&horizontalSpeed, &dfx, 4);
  memcpy(&verticalAngle, &yAngle, 4);

  //  Provide the destination vertical angle
  PowerPC::HostWrite_U32(verticalAngle, 0x804D3FFC);
  PowerPC::HostWrite_U32(verticalAngle, 0x804c10ec);

  //  I didn't investigate why, but this has to be 0
  //  it also has to do with horizontal turning, but is limited to a certain speed
  PowerPC::HostWrite_U32(0, 0x804D3D74);
  //  provide the speed to turn horizontally
  PowerPC::HostWrite_U32(horizontalSpeed, 0x804d3d38);

  // beam switching
  int beam_id = getBeamSwitch(prime_one_beams);
  if (beam_id != -1)
  {
    PowerPC::HostWrite_U32(beam_id, 0x804a79f4);
    PowerPC::HostWrite_U32(1, 0x804a79f0);
  }
  int visor_id, visor_off;
  std::tie(visor_id, visor_off) = getVisorSwitch(prime_one_visors);
  if (visor_id != -1)
  {
    u32 visor_base = PowerPC::HostRead_U32(0x804bfcd4);
    // check if we have the visor
    if (PowerPC::HostRead_U32(visor_base + (visor_off * 8) + 0x30) != 0)
    {
      PowerPC::HostWrite_U32(visor_id, visor_base + 0x1c);
    }
  }
  {
    u32 camera_ptr = PowerPC::HostRead_U32(0x804bf420 + 0x810);
    u32 camera_offset = ((PowerPC::HostRead_U32(0x804c4a08) >> 16) & 0x3ff) << 3;
    u32 camera_base = PowerPC::HostRead_U32(camera_ptr + camera_offset + 4);
    float fov = prime::GetFov();
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_base + 0x164);
    PowerPC::HostWrite_U32(*((u32*)&fov), 0x805c0e38);
    PowerPC::HostWrite_U32(*((u32*)&fov), 0x805c0e3c);
  }
}

void primeOne_PAL()
{
  // Flag which indicates lock-on
  if (PowerPC::HostRead_U8(0x804C3FF3))
  {
    PowerPC::HostWrite_U32(0, 0x804D7C78);
    return;
  }

  // for vertical angle control, we need to send the actual direction to look
  // this is a matrix LOL, clamping between ~[-1.22, 1.22] seems most effective
  // TODO FUTURE ME: use matrix math to write immediate look directions
  static float yAngle = 0;

  int dx = prime::g_mouse_input->GetDeltaHorizontalAxis(),
      dy = prime::g_mouse_input->GetDeltaVerticalAxis();

  float vSensitivity = (prime::GetSensitivity() * TURNRATE_RATIO) / (60.0f);

  float dfx = dx * -prime::GetSensitivity();
  yAngle += ((float)dy * -vSensitivity);
  yAngle = clamp(-1.22f, 1.22f, yAngle);

  u32 horizontalSpeed, verticalAngle;
  memcpy(&horizontalSpeed, &dfx, 4);
  memcpy(&verticalAngle, &yAngle, 4);

  //  Provide the destination vertical angle
  PowerPC::HostWrite_U32(verticalAngle, 0x804D7F3C);
  PowerPC::HostWrite_U32(verticalAngle, 0x804c502c);

  //  provide the speed to turn horizontally
  PowerPC::HostWrite_U32(horizontalSpeed, 0x804D7C78);

  // beam switching
  int beam_id = getBeamSwitch(prime_one_beams);
  if (beam_id != -1)
  {
    PowerPC::HostWrite_U32(beam_id, 0x804a79f4);
    PowerPC::HostWrite_U32(1, 0x804a79f0);
  }
  int visor_id, visor_off;
  std::tie(visor_id, visor_off) = getVisorSwitch(prime_one_visors);
  if (visor_id != -1)
  {
    u32 visor_base = PowerPC::HostRead_U32(0x804c3c14);
    // check if we have the visor
    if (PowerPC::HostRead_U32(visor_base + (visor_off * 8) + 0x30) != 0)
    {
      PowerPC::HostWrite_U32(visor_id, visor_base + 0x1c);
    }
  }
  {
    u32 camera_ptr = PowerPC::HostRead_U32(0x804c3360 + 0x810);
    u32 camera_offset = ((PowerPC::HostRead_U32(0x804c8948) >> 16) & 0x3ff) << 3;
    u32 camera_base = PowerPC::HostRead_U32(camera_ptr + camera_offset + 4);
    float fov = prime::GetFov();
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_base + 0x164);
    PowerPC::HostWrite_U32(*((u32*)&fov), 0x805c5178);
    PowerPC::HostWrite_U32(*((u32*)&fov), 0x805c517c);
  }
}

//*****************************************************************************************
// Metroid Prime 2
//*****************************************************************************************
void primeTwo_NTSC()
{
  // Specific to prime 2 - This find's the "camera structure" for prime 2
  u32 baseAddress = PowerPC::HostRead_U32(0x804e72e8 + 0x14f4);
  // Makes sure the baaseaddress is within the valid range of memoryaddresses for Wii
  // this is a heuristic, not a solution
  if (!mem_check(baseAddress))
  {
    return;
  }

  // static address representing if lockon pressed
  if (PowerPC::HostRead_U8(0x804e894f))
  {
    PowerPC::HostWrite_U32(0, baseAddress + 0x178);
    return;
  }
  // You give yAngle the angle you want to turn to
  static float yAngle = 0;

  // Create values for Change in X and Y mouse position
  int dx = prime::g_mouse_input->GetDeltaHorizontalAxis(),
      dy = prime::g_mouse_input->GetDeltaVerticalAxis();

  // hSensitivity - Horizontal axis sensitivity
  // vSensitivity - Vertical axis sensitivity
  float vSensitivity = (prime::GetSensitivity() * TURNRATE_RATIO) / (60.0f);

  // Rate at which we will turn by multiplying the change in x by hSensitivity.
  float dfx = dx * -prime::GetSensitivity();

  // Scale mouse movement by sensitivity
  yAngle += (float)dy * -vSensitivity;
  yAngle = clamp(-1.04f, 1.04f, yAngle);

  u32 arm_cannon_model_matrix = PowerPC::HostRead_U32(baseAddress + 0xea8) + 0x3b0;

  // HorizontalSpeed and Vertical angle to store values, used as buffers for memcpy reference
  // variables
  u32 horizontalSpeed, verticalAngle;

  // bit_casting
  memcpy(&horizontalSpeed, &dfx, 4);
  memcpy(&verticalAngle, &yAngle, 4);

  // Write the data to the addresses we want
  PowerPC::HostWrite_U32(verticalAngle, baseAddress + 0x5f0);
  PowerPC::HostWrite_U32(verticalAngle, arm_cannon_model_matrix + 0x24);
  PowerPC::HostWrite_U32(horizontalSpeed, baseAddress + 0x178);

  int beam_id = getBeamSwitch(prime_two_beams);
  if (beam_id != -1)
  {
    PowerPC::HostWrite_U32(beam_id, 0x804cd254);
    PowerPC::HostWrite_U32(1, 0x804cd250);
  }
  int visor_id, visor_off;
  std::tie(visor_id, visor_off) = getVisorSwitch(prime_two_visors);
  if (visor_id != -1)
  {
    u32 visor_base = PowerPC::HostRead_U32(baseAddress + 0x12ec);
    // check if we have the visor
    if (PowerPC::HostRead_U32(visor_base + (visor_off * 12) + 0x5c) != 0)
    {
      PowerPC::HostWrite_U32(visor_id, visor_base + 0x34);
    }
  }
  {
    u32 camera_ptr = PowerPC::HostRead_U32(0x804e72e8 + 0x810);
    u32 camera_offset = ((PowerPC::HostRead_U32(PowerPC::HostRead_U32(0x804eb9ac)) >> 16) & 0x3ff) << 3;
    u32 camera_offset_tp = ((PowerPC::HostRead_U32(PowerPC::HostRead_U32(0x804eb9ac) + 0xa) >> 16) & 0x3ff) << 3;
    u32 camera_base = PowerPC::HostRead_U32(camera_ptr + camera_offset + 4);
    u32 camera_base_tp = PowerPC::HostRead_U32(camera_ptr + camera_offset_tp + 4);
    float fov = prime::GetFov();
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_base + 0x1e8);
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_base_tp + 0x1e8);
  }
}

// TODO: try to refactor the PAL copies down, the logic is equivalent...
void primeTwo_PAL()
{
  u32 baseAddress = PowerPC::HostRead_U32(0x804ee738 + 0x14f4);
  if (!mem_check(baseAddress))
  {
    return;
  }
  if (PowerPC::HostRead_U8(0x804efd9f))
  {
    PowerPC::HostWrite_U32(0, baseAddress + 0x178);
    return;
  }

  static float yAngle = 0;

  int dx = prime::g_mouse_input->GetDeltaHorizontalAxis(),
      dy = prime::g_mouse_input->GetDeltaVerticalAxis();

  float vSensitivity = (prime::GetSensitivity() * TURNRATE_RATIO) / (60.0f);

  float dfx = dx * -prime::GetSensitivity();

  yAngle += (float)dy * -vSensitivity;
  yAngle = clamp(-1.04f, 1.04f, yAngle);

  u32 arm_cannon_model_matrix = PowerPC::HostRead_U32(baseAddress + 0xea8) + 0x3b0;
  u32 horizontalSpeed, verticalAngle;

  memcpy(&horizontalSpeed, &dfx, 4);
  memcpy(&verticalAngle, &yAngle, 4);

  PowerPC::HostWrite_U32(verticalAngle, baseAddress + 0x5f0);
  PowerPC::HostWrite_U32(verticalAngle, arm_cannon_model_matrix + 0x24);
  PowerPC::HostWrite_U32(horizontalSpeed, baseAddress + 0x178);

  int beam_id = getBeamSwitch(prime_two_beams);
  if (beam_id != -1)
  {
    PowerPC::HostWrite_U32(beam_id, 0x804cd254);
    PowerPC::HostWrite_U32(1, 0x804cd250);
  }
  int visor_id, visor_off;
  std::tie(visor_id, visor_off) = getVisorSwitch(prime_two_visors);
  if (visor_id != -1)
  {
    u32 visor_base = PowerPC::HostRead_U32(baseAddress + 0x12ec);
    if (PowerPC::HostRead_U32(visor_base + (visor_off * 12) + 0x5c) != 0)
    {
      PowerPC::HostWrite_U32(visor_id, visor_base + 0x34);
    }
  }
  {
    u32 camera_ptr = PowerPC::HostRead_U32(0x804ee738 + 0x810);
    u32 camera_offset = ((PowerPC::HostRead_U32(PowerPC::HostRead_U32(0x804f2f4c)) >> 16) & 0x3ff) << 3;
    u32 camera_offset_tp = ((PowerPC::HostRead_U32(PowerPC::HostRead_U32(0x804f2f4c) + 0xa) >> 16) & 0x3ff) << 3;
    u32 camera_base = PowerPC::HostRead_U32(camera_ptr + camera_offset + 4);
    u32 camera_base_tp = PowerPC::HostRead_U32(camera_ptr + camera_offset_tp + 4);
    float fov = prime::GetFov();
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_base + 0x1e8);
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_base_tp + 0x1e8);
  }
}

//*****************************************************************************************
// Metroid Prime 3
//*****************************************************************************************
#pragma optimize("", off)
void primeThree_NTSC()
{
  static float yAngle = 0;

  u32 baseAddressSelf = PowerPC::HostRead_U32(PowerPC::HostRead_U32(PowerPC::HostRead_U32(0x805c6c40 + 0x2c) + 0x04) + 0x2184);
  u32 baseAddressVisor = PowerPC::HostRead_U32(baseAddressSelf + 0x35a8);
  if (!mem_check(baseAddressSelf) || !mem_check(baseAddressVisor))
  {
    return;
  }

  if (PowerPC::HostRead_U8(0x805c8d77) || PowerPC::HostRead_U8(baseAddressSelf + 0x378))
  {
    u32 off1 = PowerPC::HostRead_U32(0x8066fd08);
    u32 off2 = PowerPC::HostRead_U32(off1 + 0xc54);
    handleCursor(off2 + 0x9c, off2 + 0x15c, 0.95f, 0.90f);
    return;
  }
  else
  {
    u32 off1 = PowerPC::HostRead_U32(0x8066fd08);
    u32 off2 = PowerPC::HostRead_U32(off1 + 0xc54);
    PowerPC::HostWrite_U32(0, off2 + 0x9c);
    PowerPC::HostWrite_U32(0, off2 + 0x15c);
    cursor_xPosition = 0;
    cursor_yPosition = 0;
  }

  if (PowerPC::HostRead_U8(0x805c6db7))
  {
    return;
  }
  //[[[0x2c+805c6c40]+4]+0x2184]+0x174


  int dx = prime::g_mouse_input->GetDeltaHorizontalAxis(),
    dy = prime::g_mouse_input->GetDeltaVerticalAxis();

  float vSensitivity = (prime::GetSensitivity() * TURNRATE_RATIO) / (60.0f);

  float dfx = dx * -prime::GetSensitivity();
  float dfy = (float)dy * -vSensitivity;

  yAngle += dfy;
  yAngle = clamp(-1.5f, 1.5f, yAngle);


  u32 horizontalSpeed, verticalAngle;

  memcpy(&horizontalSpeed, &dfx, 4);
  memcpy(&verticalAngle, &yAngle, 4);

  PowerPC::HostWrite_U32(horizontalSpeed, baseAddressSelf + 0x174);
  PowerPC::HostWrite_U32(0, baseAddressSelf + 0x174 + 0x18);
  u32 rtoc_min_turn_rate = GPR(2) - 0x5FF0;
  PowerPC::HostWrite_U32(0, rtoc_min_turn_rate);
  PowerPC::HostWrite_U32(verticalAngle, baseAddressSelf + 0x784);

  int visor_id, visor_off;
  std::tie(visor_id, visor_off) = getVisorSwitch(prime_three_visors);
  if (visor_id != -1)
  {
    if (PowerPC::HostRead_U32(baseAddressVisor + (visor_off * 12) + 0x58) != 0)
    {
      PowerPC::HostWrite_U32(visor_id, baseAddressVisor + 0x34);
    }
  }

  {
    u32 camera_fov = PowerPC::HostRead_U32(PowerPC::HostRead_U32(PowerPC::HostRead_U32(PowerPC::HostRead_U32(0x805c6c40 + 0x28) + 0x1010) + 0x1c) + 0x178);
    u32 camera_fov_tp = PowerPC::HostRead_U32(PowerPC::HostRead_U32(PowerPC::HostRead_U32(PowerPC::HostRead_U32(0x805c6c40 + 0x28) + 0x1010) + 0x24) + 0x178);
    float fov = prime::GetFov();
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_fov + 0x1c);
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_fov_tp + 0x1c);
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_fov + 0x18);
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_fov_tp + 0x18);
  }
}
#pragma optimize("", on)
void primeThree_PAL()
{
  static float yAngle = 0;

  u32 baseAddressSelf = PowerPC::HostRead_U32(PowerPC::HostRead_U32(PowerPC::HostRead_U32(0x805ca0c0 + 0x2c) + 0x04) + 0x2184);
  u32 baseAddressVisor = PowerPC::HostRead_U32(baseAddressSelf + 0x35a8);
  if (!mem_check(baseAddressSelf) || !mem_check(baseAddressVisor))
  {
    return;
  }

  if (PowerPC::HostRead_U8(0x805cc1d7) || PowerPC::HostRead_U8(baseAddressSelf + 0x378))
  {
    u32 off1 = PowerPC::HostRead_U32(0x80673588);
    u32 off2 = PowerPC::HostRead_U32(off1 + 0xd04);
    handleCursor(off2 + 0x9c, off2 + 0x15c, 0.95f, 0.90f);
    return;
  }
  else
  {
    u32 off1 = PowerPC::HostRead_U32(0x80673588);
    u32 off2 = PowerPC::HostRead_U32(off1 + 0xd04);
    PowerPC::HostWrite_U32(0, off2 + 0x9c);
    PowerPC::HostWrite_U32(0, off2 + 0x15c);
    cursor_xPosition = 0;
    cursor_yPosition = 0;
  }

  if (PowerPC::HostRead_U8(0x805ca237))
  {
    return;
  }

  int dx = prime::g_mouse_input->GetDeltaHorizontalAxis(),
    dy = prime::g_mouse_input->GetDeltaVerticalAxis();

  float vSensitivity = (prime::GetSensitivity() * TURNRATE_RATIO) / (60.0f);

  float dfx = dx * -prime::GetSensitivity();
  float dfy = (float)dy * -vSensitivity;

  yAngle += dfy;
  yAngle = clamp(-1.5f, 1.5f, yAngle);


  u32 horizontalSpeed, verticalAngle;

  memcpy(&horizontalSpeed, &dfx, 4);
  memcpy(&verticalAngle, &yAngle, 4);

  PowerPC::HostWrite_U32(horizontalSpeed, baseAddressSelf + 0x174);
  PowerPC::HostWrite_U32(0, baseAddressSelf + 0x174 + 0x18);
  u32 rtoc_min_turn_rate = GPR(2) - 0x6000;
  PowerPC::HostWrite_U32(0, rtoc_min_turn_rate);
  PowerPC::HostWrite_U32(verticalAngle, baseAddressSelf + 0x784);

  int visor_id, visor_off;
  std::tie(visor_id, visor_off) = getVisorSwitch(prime_three_visors);
  if (visor_id != -1)
  {
    if (PowerPC::HostRead_U32(baseAddressVisor + (visor_off * 12) + 0x58) != 0)
    {
      PowerPC::HostWrite_U32(visor_id, baseAddressVisor + 0x34);
    }
  }
  {
    u32 camera_fov = PowerPC::HostRead_U32(PowerPC::HostRead_U32(PowerPC::HostRead_U32(PowerPC::HostRead_U32(0x805ca0c0 + 0x28) + 0x1010) + 0x1c) + 0x178);
    u32 camera_fov_tp = PowerPC::HostRead_U32(PowerPC::HostRead_U32(PowerPC::HostRead_U32(PowerPC::HostRead_U32(0x805ca0c0 + 0x28) + 0x1010) + 0x24) + 0x178);
    float fov = prime::GetFov();
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_fov + 0x1c);
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_fov_tp + 0x1c);
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_fov + 0x18);
    PowerPC::HostWrite_U32(*((u32*)&fov), camera_fov_tp + 0x18);
  }
}

void beamChangeCode_mp1(std::vector<ARCode>& code_vec, u32 base_offset)
{
  ARCode c1;
  c1.active = c1.user_defined = true;

  c1.ops.push_back(AREntry(base_offset + 0x00, 0x3c80804a));
  c1.ops.push_back(AREntry(base_offset + 0x04, 0x388479f0));
  c1.ops.push_back(AREntry(base_offset + 0x08, 0x80640000));
  c1.ops.push_back(AREntry(base_offset + 0x0c, 0x2c030000));
  c1.ops.push_back(AREntry(base_offset + 0x10, 0x41820058));
  c1.ops.push_back(AREntry(base_offset + 0x14, 0x83440004));
  c1.ops.push_back(AREntry(base_offset + 0x18, 0x7f59d378));
  c1.ops.push_back(AREntry(base_offset + 0x1c, 0x38600000));
  c1.ops.push_back(AREntry(base_offset + 0x20, 0x90640000));
  c1.ops.push_back(AREntry(base_offset + 0x24, 0x48000044));
  code_vec.push_back(c1);
}

void beamChangeCode_mp2(std::vector<ARCode>& code_vec, u32 base_offset)
{
  ARCode c1;
  c1.active = c1.user_defined = true;

  c1.ops.push_back(AREntry(base_offset + 0x00, 0x3c80804d));
  c1.ops.push_back(AREntry(base_offset + 0x04, 0x3884d250));
  c1.ops.push_back(AREntry(base_offset + 0x08, 0x80640000));
  c1.ops.push_back(AREntry(base_offset + 0x0c, 0x2c030000));
  c1.ops.push_back(AREntry(base_offset + 0x10, 0x4182005c));
  c1.ops.push_back(AREntry(base_offset + 0x14, 0x83e40004));
  c1.ops.push_back(AREntry(base_offset + 0x18, 0x7ffefb78));
  c1.ops.push_back(AREntry(base_offset + 0x1c, 0x38600000));
  c1.ops.push_back(AREntry(base_offset + 0x20, 0x90640000));
  c1.ops.push_back(AREntry(base_offset + 0x24, 0x48000048));
  code_vec.push_back(c1);
}

void controlStateHook_mp3(std::vector<ARCode>& code_vec, u32 base_offset, bool ntsc)
{
  ARCode c1;
  c1.active = c1.user_defined = true;
  if (ntsc)
  {
    c1.ops.push_back(AREntry(base_offset + 0x00, 0x3C60805C));
    c1.ops.push_back(AREntry(base_offset + 0x04, 0x38636C40));
  }
  else
  {
    c1.ops.push_back(AREntry(base_offset + 0x00, 0x3C60805D));
    c1.ops.push_back(AREntry(base_offset + 0x04, 0x3863A0C0));
  }
  c1.ops.push_back(AREntry(base_offset + 0x08, 0x8063002C));
  c1.ops.push_back(AREntry(base_offset + 0x0c, 0x80630004));
  c1.ops.push_back(AREntry(base_offset + 0x10, 0x80632184));
  c1.ops.push_back(AREntry(base_offset + 0x14, 0x7C03F800));
  c1.ops.push_back(AREntry(base_offset + 0x18, 0x4D820020));
  c1.ops.push_back(AREntry(base_offset + 0x1c, 0x7FE3FB78));
  c1.ops.push_back(AREntry(base_offset + 0x20, 0x90C30078));
  c1.ops.push_back(AREntry(base_offset + 0x24, 0x4E800020));
  code_vec.push_back(c1);
}

// region 0: NTSC
// region 1: PAL
void ActivateARCodesFor(int game, int region)
{
  std::vector<ARCode> codes;
  if (region == 0)
  {
    if (game == 1)
    {
      ARCode c1;
      c1.active = true;
      c1.user_defined = true;

      c1.ops.push_back(AREntry(0x04098ee4, 0xec010072));
      c1.ops.push_back(AREntry(0x04099138, 0x60000000));
      c1.ops.push_back(AREntry(0x04183a8c, 0x60000000));
      c1.ops.push_back(AREntry(0x04183a64, 0x60000000));
      c1.ops.push_back(AREntry(0x0417661c, 0x60000000));
      c1.ops.push_back(AREntry(0x042fb5b4, 0xd23f009c));

      codes.push_back(c1);

      beamChangeCode_mp1(codes, 0x0418e544);
    }
    else if (game == 2)
    {
      ARCode c1;
      c1.active = true;
      c1.user_defined = true;

      c1.ops.push_back(AREntry(0x0408ccc8, 0xc0430184));
      c1.ops.push_back(AREntry(0x0408cd1c, 0x60000000));
      c1.ops.push_back(AREntry(0x04147f70, 0x60000000));
      c1.ops.push_back(AREntry(0x04147f98, 0x60000000));
      c1.ops.push_back(AREntry(0x04135b20, 0x60000000));
      c1.ops.push_back(AREntry(0x0408bb48, 0x60000000));
      c1.ops.push_back(AREntry(0x0408bb18, 0x60000000));
      c1.ops.push_back(AREntry(0x043054a0, 0xd23f009c));

      codes.push_back(c1);

      beamChangeCode_mp2(codes, 0x0418cc88);
    }
    else if (game == 3)
    {
      ARCode c1;
      c1.active = true;
      c1.user_defined = true;

      c1.ops.push_back(AREntry(0x04080ac0, 0xec010072));
      c1.ops.push_back(AREntry(0x0414e094, 0x60000000));
      c1.ops.push_back(AREntry(0x0414e06c, 0x60000000));
      c1.ops.push_back(AREntry(0x04134328, 0x60000000));
      c1.ops.push_back(AREntry(0x04133970, 0x60000000));
      c1.ops.push_back(AREntry(0x0400ab58, 0x4bffad29));
      c1.ops.push_back(AREntry(0x04080D44, 0x60000000));

      codes.push_back(c1);

      controlStateHook_mp3(codes, 0x04005880, true);
    }
  }
  else if (region == 1)
  {
    if (game == 1)
    {
      ARCode c1;
      c1.active = true;
      c1.user_defined = true;

      c1.ops.push_back(AREntry(0x04099068, 0xec010072));
      c1.ops.push_back(AREntry(0x040992C4, 0x60000000));
      c1.ops.push_back(AREntry(0x04183CFC, 0x60000000));
      c1.ops.push_back(AREntry(0x04183D24, 0x60000000));
      c1.ops.push_back(AREntry(0x041768B4, 0x60000000));
      c1.ops.push_back(AREntry(0x042FB84C, 0xd23f009c));

      codes.push_back(c1);

      beamChangeCode_mp1(codes, 0x0418e7dc);
    }
    if (game == 2)
    {
      ARCode c1;
      c1.active = true;
      c1.user_defined = true;

      c1.ops.push_back(AREntry(0x0408e30C, 0xc0430184));
      c1.ops.push_back(AREntry(0x0408E360, 0x60000000));
      c1.ops.push_back(AREntry(0x041496E4, 0x60000000));
      c1.ops.push_back(AREntry(0x0414970C, 0x60000000));
      c1.ops.push_back(AREntry(0x04137240, 0x60000000));
      c1.ops.push_back(AREntry(0x0408D18C, 0x60000000));
      c1.ops.push_back(AREntry(0x0408D15C, 0x60000000));
      c1.ops.push_back(AREntry(0x04307d2c, 0xd23f009c));

      codes.push_back(c1);

      beamChangeCode_mp2(codes, 0x0418e41c);
    }
    if (game == 3)
    {
      ARCode c1;
      c1.active = true;
      c1.user_defined = true;

      c1.ops.push_back(AREntry(0x04080AB8, 0xec010072));
      c1.ops.push_back(AREntry(0x0414D9E0, 0x60000000));
      c1.ops.push_back(AREntry(0x0414D9B8, 0x60000000));
      c1.ops.push_back(AREntry(0x04133C74, 0x60000000));
      c1.ops.push_back(AREntry(0x041332BC, 0x60000000));
      c1.ops.push_back(AREntry(0x0400ab58, 0x4bffad29));
      c1.ops.push_back(AREntry(0x04080D44, 0x60000000));

      codes.push_back(c1);

      controlStateHook_mp3(codes, 0x04005880, false);
    }
  }
  ApplyCodes(codes);
}

void RunAllActive()
{

  if (!SConfig::GetInstance().bEnableCheats)
    return;

  // If the mutex is idle then acquiring it should be cheap, fast mutexes
  // are only atomic ops unless contested. It should be rare for this to
  // be contested.

  u32 game_sig = PowerPC::HostRead_Instruction(0x80074000);

  int game_id = -1;
  int region_id = -1;

  static int last_game_running = -1;
  switch (game_sig)
  {
  case 0x90010024:
    game_id = 3;
    region_id = 0;
    break;
  case 0x93FD0008:
    game_id = 3;
    region_id = 1;
    break;
  case 0x480008D1:
    game_id = 0;
    region_id = 0;
    break;
  case 0x7EE3BB78:
    game_id = 0;
    region_id = 1;
    break;
  case 0x7C6F1B78:
    game_id = 1;
    region_id = 0;
    break;
  case 0x90030028:
    game_id = 1;
    region_id = 1;
    break;
  case 0x90010020:
    game_id = 2;
    {
      u32 region_diff = PowerPC::HostRead_U32(0x800CC000);
      if (region_diff == 0x981D005E)
      {
        region_id = 0;
      }
      else if (region_diff == 0x8803005D)
      {
        region_id = 1;
      }
      else
      {
        game_id = -1;
        region_id = -1;
      }
    }
    break;
  default:
    game_id = -1;
    region_id = -1;
  }

  if (game_id == 0)
  {
    if (last_game_running != 1)
    {
      prime::RefreshControlDevices();
      last_game_running = 1;
      ActivateARCodesFor(1, region_id);
    }
    if (region_id == 0)
    {
      primeOne_NTSC();
    }
    else
    {
      primeOne_PAL();
    }
  }
  else if (game_id == 1)  // TODO
  {
    if (last_game_running != 2)
    {
      prime::RefreshControlDevices();
      last_game_running = 2;
      ActivateARCodesFor(2, region_id);
    }

    if (region_id == 0)
    {
      primeTwo_NTSC();
    }
    else
    {
      primeTwo_PAL();
    }
  }
  else if (game_id == 2)
  {
    if (last_game_running != 3)
    {
      last_game_running = 3;
      ActivateARCodesFor(3, region_id);
    }

    if (region_id == 0)
    {
      primeThree_NTSC();
    }
    else if (region_id == 1)
    {
      primeThree_PAL();
    }
  }
  else if (game_id == 3)
  {
    if (region_id == 0)
    {
      primeMenu_NTSC();
    }
    else if (region_id == 1)
    {
      primeMenu_PAL();
    }
    if (last_game_running != -1)
    {
      prime::RefreshControlDevices();
      last_game_running = -1;
      ActivateARCodesFor(-1, region_id);
    }
  }

  prime::g_mouse_input->ResetDeltas();

  std::lock_guard<std::mutex> guard(s_lock);
  s_active_codes.erase(std::remove_if(s_active_codes.begin(), s_active_codes.end(),
                                      [](const ARCode& code) {
                                        bool success = RunCodeLocked(code);
                                        LogInfo("\n");
                                        return !success;
                                      }),
                       s_active_codes.end());
  s_disable_logging = true;
}

void SetActiveGame(int game)
{
  active_game = game;
}
}  // namespace ActionReplay
