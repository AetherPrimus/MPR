#pragma once

#include <wx/panel.h>
#include <wx/generic/statbmpg.h>
#include "Core/PrimeHack/TextureSwapper.h"

class PreviewWidget : public wxPanel
{
public:
  PreviewWidget(wxWindow* parent);
  void SetRGBA(unsigned char R, unsigned char G, unsigned char B, unsigned char A);
  void SetUseMinimal(bool min);
  void SetReticleImage(prime::ReticleSelection selection);
  void UpdatePreview();

private:
  void OnPaint(wxPaintEvent& event);
  void OnEraseBackground(wxEraseEvent& event);
  unsigned char R, G, B, A;
  bool useMinimal = false;
  wxBitmap background;
  wxImage hud;
  wxImage hud_min;
  wxImage reticle;
};
