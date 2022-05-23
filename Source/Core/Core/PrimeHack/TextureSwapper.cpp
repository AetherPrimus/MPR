#include "TextureSwapper.h"

#include <vector>

#include "Core/ConfigManager.h"
#include "Core/PrimeHack/HackConfig.h"

#include "Common/FileUtil.h"
#include "Common/CommonPaths.h"

#include "VideoCommon/HiresTextures.h"

namespace prime {
  std::vector<std::string> texture_queue;
  std::unordered_set<std::string> daynight_textures;
  std::unordered_set<std::string> weathering_textures;

  Time time = DAY;
  Weathering weathering = NONE;
  ReticleSelection reticle = MPR;

  std::vector<std::string>& GetInvalidateQueue() {
    return texture_queue;
  }

  void AddInvalidateTexture(std::string texture) {
    texture_queue.emplace_back(texture);
  }

  void ClearInvalidateQueue() {
    texture_queue.clear();

    HiresTexture::Update();
  }

  void SwitchBeamCursor(int beam_id) {
    SetCurrentBeam(beam_id);
    
    HiresTexture::Update(std::vector<std::string> { "/tex1_128x128_847479ca4ca13986_14.adds" });
  }

  std::string GetAlternateTexture(std::string& fileitem) {
    std::string texture_folder = GetTextureFolder();
    if (texture_folder.empty()) return fileitem;

    if (StringEndsWith(fileitem, "tex1_128x128_847479ca4ca13986_14.adds")) {
      if (reticle == Standard)
        return fileitem;
      else if (reticle == Dot)
        return "/Cursors/Reticule_DOT.png";
      else if (reticle == None)
        return "/Cursors/Reticule_NONE.png";

      std::string cursor = CheckBeamSwitch(texture_folder);

      if (!cursor.empty()) return cursor;
      else return fileitem;
    }

    std::string folder;
    std::string filename;
    SplitFolder(fileitem, &folder, &filename);

    if (!folder.empty()) {
      std::string t_prefix;
      bool dusk = File::Exists(folder + "dusk_" + filename);
      bool night = File::Exists(folder + "night_" + filename);

      if (dusk || night) {
        daynight_textures.insert(fileitem);

        if (time == DAY) {
          return fileitem;
        } else if (dusk && time == DUSK) {
          return folder + "dusk_" + filename;
        }
        else if (night && time == DAY) {
          return folder + "night_" + filename;
        }
      }

      bool lightdirt = File::Exists(folder + "lightdirt_" + filename);
      bool heavydirt = File::Exists(folder + "heavydirt_" + filename);
      if (lightdirt || heavydirt) {
        weathering_textures.insert(fileitem);

        if (weathering == NONE) {
          return fileitem;
        } else if (lightdirt && weathering == LIGHT_DIRT) {
          return folder + "lightdirt_" + filename;
        }
        else if (heavydirt && weathering == HEAVY_DIRT) {
          return folder + "heavydirt_" + filename;
        }
      } else {
        return fileitem;
      }
    }

    return fileitem;
  }

  std::string CheckBeamSwitch(std::string texture_folder) {
    int i = prime::GetCurrentBeam();
    if (i == 0) {
      std::string cursor = texture_folder + "/Cursors/Reticule_DOT.png";

      if (File::Exists(cursor)) return cursor;
      else return "";
    } else if (i == 1) {
      std::string cursor = texture_folder + "/Cursors/Reticule_ICE.png";

      if (File::Exists(cursor)) return cursor;
      else return "";
    } else if (i == 2) {
      std::string cursor = texture_folder + "/Cursors/Reticule_WAVE.png";

      if (File::Exists(cursor)) return cursor;
      else return "";
    } else if (i == 3) {
      std::string cursor = texture_folder + "/Cursors/Reticule_PLASMA.png";

      if (File::Exists(cursor)) return cursor;
      else return "";
    }

    return "";
  }

  void SetReticle(ReticleSelection sel)
  {
    reticle = sel;
    HiresTexture::Update(std::vector<std::string>{"/tex1_128x128_847479ca4ca13986_14.adds"});
  }

  void IncreaseTime()
  {
    if (time == DAY) time = DUSK;
    else if (time == DUSK) time = NIGHT;
    else if (time == NIGHT) time = DAY;

    std::vector<std::string> v;
    v.reserve(daynight_textures.size());
    v.insert(v.end(), daynight_textures.begin(), daynight_textures.end());

    HiresTexture::Update(v);
  }

  int GetTime()
  {
    return time;
  }

  void CycleWeathering()
  {
    if (weathering == NONE) weathering = LIGHT_DIRT;
    else if (weathering == LIGHT_DIRT) weathering = HEAVY_DIRT;
    else if (weathering == HEAVY_DIRT) weathering = NONE;

    std::vector<std::string> v;
    v.reserve(weathering_textures.size());
    v.insert(v.end(), weathering_textures.begin(), weathering_textures.end());

    HiresTexture::Update(v);
  }

  int GetWeathering()
  {
    return weathering;
  }

  bool StringEndsWith(const std::string& str, const std::string& end)
  {
    return str.size() >= end.size() && std::equal(end.rbegin(), end.rend(), str.rbegin());
  }

  void SplitFolder(std::string& filepath, std::string* folder, std::string* filename) {
    size_t pos = filepath.find_last_of("/" ":");

    if (std::string::npos == pos)
      pos = 0;
    else
      pos += 1;

    *folder = filepath.substr(0, pos);
    *filename = filepath.substr(pos, filepath.length());
  }

  std::string GetTextureFolder() {
    std::string texture_directory =
        File::GetSysDirectory() + "AetherLabs" + DIR_SEP + "UI" + DIR_SEP;

    if (!File::Exists(texture_directory))
      return "";

    return texture_directory;
  }
}
