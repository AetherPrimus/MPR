#pragma once

#include <string>

namespace prime {
  enum Time {
    DAY,
    DUSK,
    NIGHT
  };

  enum Weathering {
    NONE,
    LIGHT_DIRT,
    HEAVY_DIRT
  };

  enum ReticleSelection : long
  {
    MPR,
    Dot,
    Standard,
    None
  };

  std::vector<std::string> GetInvalidateQueue();
  void AddInvalidateTexture(std::string texture);
  void ClearInvalidateQueue();

  void SwitchBeamCursor(int beam_id);
  std::string GetAlternateTexture(std::string& fileItem);
  std::string CheckBeamSwitch(std::string texture_folder);

  void SetReticle(ReticleSelection sel);

  void IncreaseTime();
  int GetTime();

  void CycleWeathering();
  int GetWeathering();

  std::string GetTextureFolder();
  void SplitFolder(std::string& filepath, std::string* folder, std::string* filename);
  bool StringEndsWith(const std::string& str, const std::string& end);
}
