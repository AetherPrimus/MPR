#pragma once

#include <thread>
#include <mutex>
#include <filesystem>
#include <map>

#ifdef _WIN32
#include <Windows.h>
#include <Winternl.h>
#include <debugapi.h>
#include <psapi.h>
#include <tchar.h>
#endif

#include "Common/File.h"
#include "VideoCommon/HiresTextures.h"
#include "DDSLoader.h"

#define PAKVERSION 1

namespace Aether {
#pragma pack(push, 1)
  struct Header {
    const char magic[3]{ 'A', 'P', ' ' };
    char name[12] = "";
    uint32_t priority = 1;
    uint32_t pakversion = PAKVERSION;
    uint32_t file_count = 0;
  };

  struct Entry
  {
    uint64_t id, file_length, name_length;
  };
#pragma pack(pop)

  struct DLCInfo
  {
    std::string display_name;
    std::string description;
    const char* preview_data;
  };

  struct AFile : public File::IFile {
    std::string name;
    size_t file_size = 0;
    size_t file_pointer = 0;
    const char* buffer;

    AFile(const std::string name, size_t size, const char* buf) : name(name), file_size(size), buffer(buf) {}
    AFile() {}

    bool Open(const std::string& filename, const char openmode[]);
    bool ReadBytes(void* data, size_t length);
    void RetrieveHeader(DDSHeader* ddsd);
    bool RetrieveDataPtr(u8** ptr, size_t length);

    inline u64 GetSize() const {
      return file_size;
    }
  };

  static std::mutex progress_mutex;
  static std::mutex init_mutex;

  struct AetherPak {
    u32 priority;
    const char* name;
    std::string path;

    bool pak_initialised = false;   
    std::map<std::string, AFile*> file_map;
    std::string buffer_container;

    DLCInfo* dlc_info = nullptr;

    bool GetFileByName(std::string, AFile* out);
    const std::map<std::string, AFile*>* GetAllFiles();
    bool LoadPak(std::string path);
    bool ProcessFiles(const char* buffer, size_t size);
    bool TryGetDLC();
  };

  void InitPaks();
  void ShutDown();

  void AddPak(std::shared_ptr<AetherPak> pak_ptr);
  void EnablePak(std::shared_ptr<AetherPak> pak_ptr);
  void ClearActivePaks();
  std::vector<std::shared_ptr<AetherPak>> GetPaks();

  void PeriodicallyCheckSnoopers();

  inline bool TestForSnoopers()
  {
#ifdef _WIN32
    PTEB tebPtr = reinterpret_cast<PTEB>(
        __readgsqword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)));
    if (tebPtr->ProcessEnvironmentBlock->BeingDebugged)
    {
      if (g_texture_cache)
        g_texture_cache->Invalidate();

      ShutDown();
      return false;
    }

    HMODULE hMods[1024];
    HANDLE hProcess = GetCurrentProcess();
    DWORD cbNeeded;

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
      for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
      {
        TCHAR szModName[MAX_PATH];

        if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
        {
          std::wstring str = std::wstring(szModName);
          if (str.find(L"veh") != std::wstring::npos)
          {
            if (g_texture_cache)
              g_texture_cache->Invalidate();

            ShutDown();
            return false;
          }
        }
      }
    }
#endif

    return true;
  }
  } // namespace Aether
