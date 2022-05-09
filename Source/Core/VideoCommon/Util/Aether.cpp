#include "Aether.h"

#include <string>
#include "Core/Host.h"
#include "Core/Core.h"
#include "VideoCommon/TextureCacheBase.h"
#include "VideoCommon/VideoConfig.h"

namespace Aether {

std::vector<std::shared_ptr<AetherPak>> paks;
static std::thread* snooper_thread = nullptr;

const std::string PREFETCH_MSG = "Prefetching MPR's content for the next world..";

bool AetherPak::GetFileByName(std::string, AFile* out) {
  auto file = file_map.find(name);

  if (file == file_map.end())
    return false;

  out = file->second;

  return true;
}

const std::map<std::string, AFile*>* AetherPak::GetAllFiles() {
  return &file_map;
}

bool AetherPak::LoadPak(std::string path) {
  if (pak_initialised)
    return true;

  if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
    return false;

  if (!TestForSnoopers())
    return false;

  if (snooper_thread == nullptr || !snooper_thread->joinable())
  {
    snooper_thread = new std::thread(PeriodicallyCheckSnoopers);
  }

  this->path = path;

  std::unique_lock<std::mutex> progress_lock = std::unique_lock(progress_mutex, std::try_to_lock);

  if (g_ActiveConfig.bWaitForCacheHiresTextures && progress_lock.owns_lock())
  {
    Host_UpdateProgressDialog("Prefetching Package, this may take up to a minute..", 0, 1);
  }

  try
  {
    CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);

    CryptoPP::FileSource fs(path.c_str(), false);
    buffer_container.reserve(fs.MaxRetrievable());

    CryptoPP::ArraySink as(iv, iv.size());
    fs.Attach(new CryptoPP::Redirector(as));
    fs.Pump(CryptoPP::AES::BLOCKSIZE);

    unsigned char keyU[32] = { MASTERKEYU };
    for (int i = 0; i < 32 - MASTERKEYL_SZ; i++) {
      if (keyU[i] % 2 == 0) {
          keyU[i] = keyU[i] / 2;
      } else  {
          keyU[i] = keyU[i] << 2;
      }
    }

    unsigned char keyL[] = { MASTERKEYL };
    for (int i = 0; i < MASTERKEYL_SZ - 1; i++) {
      keyL[i] = keyL[i] + iv[i > 15 ? i - 15 : i];
    }

    memcpy(&keyU[32 - MASTERKEYL_SZ], &keyL[0], MASTERKEYL_SZ);

    CryptoPP::GCM<CryptoPP::AES, CryptoPP::GCM_64K_Tables>::Decryption d;
    d.SetKeyWithIV(keyU, 32, iv, iv.size());

    // More efficient than copying from bytequeues w/ ArraySource
    CryptoPP::StringSink ss(buffer_container);
    CryptoPP::AuthenticatedDecryptionFilter* filter = new CryptoPP::AuthenticatedDecryptionFilter(d,
      new CryptoPP::Redirector(ss),
      CryptoPP::AuthenticatedDecryptionFilter::DEFAULT_FLAGS, 12
    ); // AuthenticatedDecryptionFilter

    fs.Detach(filter);

    std::istream* stream = const_cast<CryptoPP::FileSource&>(fs).GetStream();
    std::ifstream::pos_type prev = stream->tellg();
    std::ifstream::pos_type end = stream->seekg(0, std::ios_base::end).tellg();
    stream->seekg(prev);

    constexpr size_t CHUNK_SZ = 32 * 1000; // 32kb // 4 * 1000 * 1000; // 4mb
    size_t notify = 5;
    size_t total = (static_cast<uint64_t>(end) - iv.size()) / CHUNK_SZ;
    size_t count = 0;
    size_t remaining = total;
    while (remaining) {
      fs.Pump(CHUNK_SZ);

      count++;
      remaining--;

      if (g_ActiveConfig.bWaitForCacheHiresTextures)
      {
        if ((count * 100) / total >= notify)
        {
          if (progress_lock.owns_lock() || progress_lock.try_lock())
          {
            Host_UpdateProgressDialog(PREFETCH_MSG.c_str(), static_cast<int>(count),
                                      static_cast<int>(total));
          }

          notify += 5;
        }         
      }    
    }

    fs.PumpAll();
    filter->Flush(false);

    if (g_ActiveConfig.bWaitForCacheHiresTextures && progress_lock.owns_lock())
    {
      Host_UpdateProgressDialog("", -1, -1);
    }

    if (filter->GetLastResult() == true) {
      if (!ProcessFiles(&buffer_container[0], buffer_container.size()))
        return false;
    }
    else {
      return false;
    }
  }
  catch (CryptoPP::Exception& e) {
    std::cerr << e.what() << std::endl;
    exit(1);
  }

  if (progress_lock.owns_lock()) {
    progress_lock.unlock();
  }
;
  pak_initialised = true;
  return true;
}

bool AetherPak::ProcessFiles(const char* buffer, size_t size) {
  if (!TestForSnoopers())
    return false;

  const Header* h = reinterpret_cast<const Header*>(buffer);
  if (h->file_count == 0)
    return false;

  name = h->name;
  priority = h->priority;

  if (h->pakversion != PAKVERSION) {
    return false;
  }

  uint64_t size_read = 0;
  uint64_t file_count = h->file_count;
  uint64_t file_pointer = sizeof(Header);

  for (uint32_t i = 0; i < file_count; i++) {
    if (file_pointer > size) {
      std::cout << "File Pointer larger than buffer" << std::endl;
      return false;
    }

    const Entry* e = reinterpret_cast<const Entry*>(&buffer[file_pointer]);
    file_pointer += sizeof(Entry);

    if (e->id != i) {
      std::cout << "Archive is corrupt." << std::endl;
      return false;
    }

    const char* name = &buffer[file_pointer];
    file_pointer += e->name_length;

    const std::string str_name = std::string(name, e->name_length);
    AFile* f = new AFile(str_name, e->file_length, &buffer[file_pointer]);
    file_map.emplace(str_name, f);

    // Skip past data section.
    file_pointer += e->file_length;
    size_read += e->file_length; 
  }

  return true;
}

bool AFile::Open(const std::string& filename, const char openmode[]) {
  std::string name = filename;
  name.push_back('\0');
  for (auto& pak : paks) {
    auto file = pak->file_map.find(name);
    if (file != pak->file_map.end()) {
      buffer = file->second->buffer;
      file_size = file->second->file_size;
      name = file->second->name;

      return file_size != 0;
    }
  }

  return false;
}

bool AFile::ReadBytes(void* data, size_t length)
{
  if (length > file_size - file_pointer)
    return false;

  memcpy(data, &buffer[file_pointer], length);
  file_pointer += length;

  return true;
}

void AFile::RetrieveHeader(DDSHeader* ddsd) {
  if (sizeof(DDSHeader) != sizeof(ADDSHeader))
    static_assert("DDS/ADDS headers are not equally sized.");

  const ADDSHeader* addsd = reinterpret_cast<const ADDSHeader*>(buffer);
  file_pointer += sizeof(ADDSHeader);

  ddsd->dwFlags = addsd->dwFlags;
  ddsd->dwHeight = addsd->dwHeight;
  ddsd->dwWidth = addsd->dwWidth;
  ddsd->dwSignature = ADDS_SIGNARURE;
  ddsd->dwSize = addsd->dwSize;
  ddsd->lPitch = addsd->lPitch;
  ddsd->dwLinearSize = addsd->dwLinearSize;
  ddsd->dwBackBufferCount = addsd->dwBackBufferCount;
  ddsd->dwDepth = addsd->dwDepth;
  ddsd->dwMipMapCount = addsd->dwMipMapCount;
  ddsd->dwRefreshRate = addsd->dwRefreshRate;
  ddsd->dwSrcVBHandle = addsd->dwSrcVBHandle;
  ddsd->dwAlphaBitDepth = addsd->dwAlphaBitDepth;
  ddsd->dwReserved = addsd->dwReserved;
  ddsd->lpSurface = addsd->lpSurface;
  ddsd->ddckCKDestOverlay = addsd->ddckCKDestOverlay;
  ddsd->dwEmptyFaceColor = addsd->dwEmptyFaceColor;
  ddsd->ddckCKDestBlt = addsd->ddckCKDestBlt;
  ddsd->ddckCKSrcOverlay = addsd->ddckCKSrcOverlay;
  ddsd->ddckCKSrcBlt = addsd->ddckCKSrcBlt;
  ddsd->ddpfPixelFormat = addsd->ddpfPixelFormat;
  ddsd->dwFVF = addsd->dwFVF;
  ddsd->dwTextureStage = addsd->dwTextureStage;
  ddsd->ddsCaps = addsd->ddsCaps;
}

bool AFile::RetrieveDataPtr(u8** ptr, size_t length)
{
  if (length > file_size - file_pointer)
    return false;

  *ptr = (u8*) &buffer[file_pointer];
  file_pointer += length;

  return true;
}

void ShutDown() {
  paks.clear();
}

void AddPak(std::shared_ptr<AetherPak> pak_ptr) {
  paks.emplace_back(std::move(pak_ptr));
}

std::vector<std::shared_ptr<AetherPak>> GetPaks()
{
  return paks;
}

void PeriodicallyCheckSnoopers() {
  while (Core::IsRunning()) {
    TestForSnoopers();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
};
