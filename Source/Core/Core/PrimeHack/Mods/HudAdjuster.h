#pragma once

#include "Core/PrimeHack/PrimeMod.h"
#include "Core/PrimeHack/PrimeUtils.h"
#include "Core/PrimeHack/TextureSwapper.h"
#include "Core/ConfigManager.h"

namespace prime
{

struct TweakAddress
{
  uint32_t address, var;

  TweakAddress(uint32_t a1, uint32_t v1) {
    address = a1;
    var = v1;
  }
};

class HudAdjuster : public PrimeMod
{
private:
  std::vector<TweakAddress> original_colours;
  u32 current_colour = -1;
  s32 zoom_factor = -1;
  bool minimal_mode = false;

public:
  u32 multiply_rgb(u32 original, float value) {
    u32 r1 = (original & 0xFF000000) >> 24;
    u32 g1 = (original & 0x00FF0000) >> 16;
    u32 b1 = (original & 0x0000FF00) >> 8;
    u32 a1 = (original & 0x000000FF);

    return std::clamp((u32) (r1 * value), 0u, 0xFFu) << 24 |
           std::clamp((u32) (g1 * value), 0u, 0xFFu) << 16 |
           std::clamp((u32) (b1 * value), 0u, 0xFFu) << 8  |
           std::clamp((u32) (a1 * value), 0u, 0xFFu);
  }

  void run_mod(Game game, Region region) override
  {
    if (game != Game::PRIME_1 && game != Game::PRIME_1_GCN)
      return;

    if (region != Region::NTSC_U && region != Region::PAL)
      return;

    LOOKUP_DYN(player);
    if (player == 0)
    {
      return;
    }

    SConfig& settings = SConfig::GetInstance();

    if (current_colour == settings.m_mpr_primary_hudcolour
        && minimal_mode == settings.m_mpr_minimal_mode
        && zoom_factor == settings.m_mpr_hud_zoom_factor)
      return;

    current_colour = settings.m_mpr_primary_hudcolour;
    minimal_mode = settings.m_mpr_minimal_mode;
    zoom_factor = settings.m_mpr_hud_zoom_factor;

    LOOKUP(tweak_gui_colors);
    LOOKUP(tweak_gui);

    // Handle posisble underflow.
    u32 empty_colour = multiply_rgb(current_colour, 0.5f);
    u32 shadow_colour = multiply_rgb(current_colour, 0.8f);
    u32 highlight_colour = multiply_rgb(current_colour, 1.2f);

    // Reset value, picked for being visually close to the original in the preview
    if (SConfig::GetInstance().m_mpr_primary_hudcolour == 0x5EABDFFF)
    {
      for (const auto& colour : original_colours)
      {
        write32(colour.var, colour.address);
      }

      // todo: fix DRY violation
      write32(minimal_mode ? 0 : current_colour, tweak_gui_colors + 0x1C);  // Combat Visor Frame
      write32(minimal_mode ? 0 : shadow_colour,
              tweak_gui_colors + 0x88);  // Visor Decoration (Ticks)

      write32(minimal_mode ? 0 : current_colour, tweak_gui_colors + 0x8);  // Radar background

      write32(minimal_mode ? 0 : 0x4b7ea366, tweak_gui_colors + 0xC);   // Radar player
      write32(minimal_mode ? 0 : 0xff6705c8, tweak_gui_colors + 0x10);  // Radar enemy

      write32(0x28, tweak_gui + 0xac); // HUD Cam Y 

      return;
    }

    write32(static_cast<u32>(0x28 - zoom_factor), tweak_gui + 0xac); // HUD Cam Y 

    write32(minimal_mode ? 0 : current_colour, tweak_gui_colors + 0x1C);  // Combat Visor Frame
    write32(minimal_mode ? 0 : shadow_colour, tweak_gui_colors + 0x88);  // Visor Decoration (Ticks)

    write32(minimal_mode ? 0 : current_colour, tweak_gui_colors + 0x8);  // Radar background

    write32(minimal_mode ? 0 : 0x4b7ea366, tweak_gui_colors + 0xC);   // Radar player
    write32(minimal_mode ? 0 : 0xff6705c8, tweak_gui_colors + 0x10);  // Radar enemy

    write32(current_colour, tweak_gui_colors + 0x8c);   // Helmet light

    write32(current_colour, tweak_gui_colors + 0x1C8);  // Combat Health Bar Full
    write32(empty_colour, tweak_gui_colors + 0x1CC);    // Combat Health Bar Empty
    write32(shadow_colour, tweak_gui_colors + 0x1D0);   // Combat Health Bar Shadow
    write32(current_colour, tweak_gui_colors + 0x1D4);  // Combat Energy Filled
    write32(empty_colour, tweak_gui_colors + 0x1D8);    // Combat Energy Empty
    write32(current_colour, tweak_gui_colors + 0x1DC);  // Combat Font
    write32(shadow_colour, tweak_gui_colors + 0x1E0);   // Combat Font Outline


    write32(current_colour, tweak_gui_colors + 0x68);  // Warning Shadow
    write32(shadow_colour, tweak_gui_colors + 0x6C);   // Warning Shadow
    write32(empty_colour, tweak_gui_colors + 0x70);    // Warning Empty
    write32(shadow_colour, tweak_gui_colors + 0x90);   // Warning Icon Neutral
    write32(current_colour, tweak_gui_colors + 0x80);  // Warning Icon Full

    write32(current_colour, tweak_gui_colors + 0x28);    // Missile Icon Active
    write32(shadow_colour, tweak_gui_colors + 0x94);     // Missile Icon Inactive
    write32(highlight_colour, tweak_gui_colors + 0x98);  // Missile Icon Charged w/ Alt
    write32(highlight_colour, tweak_gui_colors + 0x9c);  // Missile Icon Charged No alt
    write32(empty_colour, tweak_gui_colors + 0xa0);    // Missile Icon Empty No Alt
    write32(current_colour, tweak_gui_colors + 0x74);  // Missile Full
    write32(shadow_colour,  tweak_gui_colors + 0x78);  // Missile Shadow
    write32(empty_colour,   tweak_gui_colors + 0x7C);  // Missile Empty
    write32(current_colour, tweak_gui_colors + 0x1A4); // Missile Font
    write32(shadow_colour, tweak_gui_colors + 0x1A8);  // Missile Font Outline

    write32(highlight_colour, tweak_gui_colors + 0x2C);  // Beam/Visor Icons
    write32(shadow_colour, tweak_gui_colors + 0x30);   // Beam/Visor Icons (Inactive)
    write32(shadow_colour, tweak_gui_colors + 0xB0);   // Beam/Visor Icons (Inactive?)
    
    write32(current_colour, tweak_gui_colors + 0x148);  // Ball bomb filled
    write32(empty_colour, tweak_gui_colors + 0x14c);  // Ball bomb empty
    write32(shadow_colour, tweak_gui_colors + 0x158);   // Ball energy deco
    write32(shadow_colour, tweak_gui_colors + 0x15c);  // Ball bomb deco
    write32(current_colour, tweak_gui_colors + 0x238);  // Ball Health Bar Full
    write32(empty_colour, tweak_gui_colors + 0x23C);    // Ball Health Bar Empty
    write32(shadow_colour, tweak_gui_colors + 0x240);   // Ball Health Bar Shadow
    write32(current_colour, tweak_gui_colors + 0x244);  // Ball Energy Filled
    write32(empty_colour, tweak_gui_colors + 0x248);    // Ball Energy Empty
    write32(current_colour, tweak_gui_colors + 0x24C);  // Ball Font
    write32(shadow_colour, tweak_gui_colors + 0x250);   // Ball Font Outline

    write32(current_colour, tweak_gui_colors + 0x1E4);  // Scan Health Bar Full
    write32(empty_colour, tweak_gui_colors + 0x1E8);    // Scan Health Bar Empty
    write32(shadow_colour, tweak_gui_colors + 0x1EC);   // Scan Health Bar Shadow
    write32(current_colour, tweak_gui_colors + 0x1F0);  // Scan Energy Filled
    write32(empty_colour, tweak_gui_colors + 0x1F4);    // Scan Energy Empty
    write32(current_colour, tweak_gui_colors + 0x1F8);  // Scan Font
    write32(shadow_colour, tweak_gui_colors + 0x1FC);   // Scan Font Outline

    write32(shadow_colour, tweak_gui_colors + 0xC8);  // Scan Frame Inactive
    write32(current_colour, tweak_gui_colors + 0xCC);  // Scan Frame Active
    write32(empty_colour, tweak_gui_colors + 0xD0);  // Scan Impulse?
  }

  void add_original_colour(u32 address) {
    original_colours.emplace_back(address, read32(address));
  }

  bool init_mod(Game game, Region region) override
  {
    if (game != Game::PRIME_1 && game != Game::PRIME_1_GCN)
      return false;

    if (region != Region::NTSC_U && region != Region::PAL)
      return false;

    LOOKUP_DYN(player);
    if (player == 0)
    {
      return false;
    }

    LOOKUP(tweak_gui_colors);
    add_original_colour(tweak_gui_colors + 0x8c);  // Helmet light
    add_original_colour(tweak_gui_colors + 0x88);   // Visor Decoration (Ticks)
    add_original_colour(tweak_gui_colors + 0x1C);   // Combat Visor Frame
    add_original_colour(tweak_gui_colors + 0x1C8);  // Combat Health Bar Full
    add_original_colour(tweak_gui_colors + 0x1CC);  // Combat Health Bar Empty
    add_original_colour(tweak_gui_colors + 0x1D0);  // Combat Health Bar Shadow
    add_original_colour(tweak_gui_colors + 0x1D4);  // Combat Energy Filled
    add_original_colour(tweak_gui_colors + 0x1D8);  // Combat Energy Empty
    add_original_colour(tweak_gui_colors + 0x1DC);  // Combat Font
    add_original_colour(tweak_gui_colors + 0x1E0);  // Combat Font Outline

    add_original_colour(tweak_gui_colors + 0x8);  // Radar background

    add_original_colour(tweak_gui_colors + 0xC);   // Radar player
    add_original_colour(tweak_gui_colors + 0x10);  // Radar enemy

    add_original_colour(tweak_gui_colors + 0x28);  // Missile Icon Active
    add_original_colour(tweak_gui_colors + 0x94);  // Missile Icon Inactive
    add_original_colour(tweak_gui_colors + 0x98);  // Missile Icon Charged w/ Alt
    add_original_colour(tweak_gui_colors + 0x9c);  // Missile Icon Charged No alt
    add_original_colour(tweak_gui_colors + 0xa0);  // Missile Icon Empty No Alt
    add_original_colour(tweak_gui_colors + 0x74);  // Missile Full
    add_original_colour(tweak_gui_colors + 0x78);  // Missile Shadow
    add_original_colour(tweak_gui_colors + 0x7C);  // Missile Empty
    add_original_colour(tweak_gui_colors + 0x1A4); // Missile Font
    add_original_colour(tweak_gui_colors + 0x1A8); // Missile Font Outline

    add_original_colour(tweak_gui_colors + 0x68);  // Warning Shadow
    add_original_colour(tweak_gui_colors + 0x6C);  // Warning Shadow
    add_original_colour(tweak_gui_colors + 0x70);  // Warning Empty
    add_original_colour(tweak_gui_colors + 0x90);  // Warning Icon Neutral
    add_original_colour(tweak_gui_colors + 0x80);  // Warning Icon

    add_original_colour(tweak_gui_colors + 0xB0);  // Beam/Visor Icons (Inactive?)
    add_original_colour(tweak_gui_colors + 0x30);  // Beam/Visor Icons (Inactive)
    add_original_colour(tweak_gui_colors + 0x2C);  // Beam/Visor Icons (Active)

    add_original_colour(tweak_gui_colors + 0x148);  // Ball bomb filled
    add_original_colour(tweak_gui_colors + 0x14c);  // Ball bomb empty
    add_original_colour(tweak_gui_colors + 0x158);  // Ball energy
    add_original_colour(tweak_gui_colors + 0x15c);  // Ball bomb
    add_original_colour(tweak_gui_colors + 0x238);  // Ball Health Bar Full
    add_original_colour(tweak_gui_colors + 0x23C);  // Ball Health Bar Empty
    add_original_colour(tweak_gui_colors + 0x240);  // Ball Health Bar Shadow
    add_original_colour(tweak_gui_colors + 0x244);  // Ball Energy Filled
    add_original_colour(tweak_gui_colors + 0x248);  // Ball Energy Empty
    add_original_colour(tweak_gui_colors + 0x24C);  // Ball Font
    add_original_colour(tweak_gui_colors + 0x250);  // Ball Font Outline

    add_original_colour(tweak_gui_colors + 0x1E4);  // Scan Health Bar Full
    add_original_colour(tweak_gui_colors + 0x1E8);  // Scan Health Bar Empty
    add_original_colour(tweak_gui_colors + 0x1EC);  // Scan Health Bar Shadow
    add_original_colour(tweak_gui_colors + 0x1F0);  // Scan Energy Filled
    add_original_colour(tweak_gui_colors + 0x1F4);  // Scan Energy Empty
    add_original_colour(tweak_gui_colors + 0x1F8);  // Scan Font
    add_original_colour(tweak_gui_colors + 0x1FC);  // Scan Font Outline

    add_original_colour(tweak_gui_colors + 0xC8);  // Scan Frame Inactive
    add_original_colour(tweak_gui_colors + 0xCC);  // Scan Frame Active
    add_original_colour(tweak_gui_colors + 0xD0);  // Scan Impulse?


    return true;
  }

  void on_state_change(ModState old_state) override {
    // Triggers a refresh if the game is restarted.
    current_colour = 0;
  }
};
}  // namespace prime
