// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <string>
#include "Common/Version.h"

namespace Common
{
#define PRIMEHACK_VERSION_STR "Phaze 1 [PrimeHack-Ishiiruka]"

#ifdef _DEBUG
#define BUILD_TYPE_STR "Debug "
#elif defined DEBUGFAST
#define BUILD_TYPE_STR "DebugFast "
#else
#define BUILD_TYPE_STR ""
#endif

const std::string scm_rev_str = "MPR"

#ifdef __INTEL_COMPILER
" " PRIMEHACK_VERSION_STR " -ICC";
#else
" " PRIMEHACK_VERSION_STR;
#endif

const std::string scm_rev_git_str = "";
const std::string scm_rev_cache_str = "202007302245";
const std::string scm_desc_str = "";
const std::string scm_branch_str = "";
const std::string scm_distributor_str = "";

#ifdef _WIN32
const std::string netplay_dolphin_ver = " Win";
#elif __APPLE__
const std::string netplay_dolphin_ver = " Mac";
#else
const std::string netplay_dolphin_ver = " Lin";
#endif


}
