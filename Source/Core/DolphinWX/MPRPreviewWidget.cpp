#include "MPRPreviewWidget.h"

#include "wx/dcbuffer.h"
#include "DolphinWX/WxUtils.h"

#include "Common/FileUtil.h"
#include "Common/CommonPaths.h"

PreviewWidget::PreviewWidget(wxWindow* parent) : wxPanel(parent)
{
  background = WxUtils::LoadScaledResourceBitmap("preview_background", this);
  hud = WxUtils::LoadScaledResourceBitmap("hud_template", this).ConvertToImage();
  hud_min = WxUtils::LoadScaledResourceBitmap("hud_template_min", this).ConvertToImage();

  this->SetMinSize(*new wxSize(720, 540));
  this->Connect(wxID_ANY, wxEVT_PAINT, wxPaintEventHandler(PreviewWidget::OnPaint),
               (wxObject*) 0, this);
  this->Connect(wxID_ANY, wxEVT_ERASE_BACKGROUND,
                wxEraseEventHandler(PreviewWidget::OnEraseBackground), (wxObject*) 0, this);
}

void PreviewWidget::SetRGBA(unsigned char R, unsigned char G, unsigned char B, unsigned char A)
{
  this->R = R;
  this->B = B;
  this->G = G;
  this->A = A;
}

void PreviewWidget::SetUseMinimal(bool min) {
  useMinimal = min;
}

void PreviewWidget::SetReticleImage(prime::ReticleSelection selection)
{
  std::string name =
    File::GetSysDirectory() + "AetherLabs" + DIR_SEP +
      "UI" + DIR_SEP + "Cursors" + DIR_SEP;

  switch (selection) {
  case prime::ReticleSelection::MPR:
    name += "Reticule_DOT.png";
    break;
  case prime::ReticleSelection::Standard:
    name += "Reticule_POWER.png";
    break;
  case prime::ReticleSelection::Dot:
    name += "Reticule_DOT.png";
    break;
  case prime::ReticleSelection::None:
    name += "Reticule_NONE.png";
    break;
  }

  reticle = WxUtils::LoadScaledBitmap(name, this).ConvertToImage();
  reticle = reticle.Scale(80, 80, wxImageResizeQuality::wxIMAGE_QUALITY_HIGH);
  reticle = reticle.Rotate180();
}

void PreviewWidget::UpdatePreview() {
  this->Refresh();
  this->Update();
}

void PreviewWidget::OnPaint(wxPaintEvent& event) {
  wxBufferedPaintDC dc(this);

  wxImage image = useMinimal ? hud_min : hud;
  for (int x = 0; x < image.GetWidth(); x++)
  {
    for (int y = 0; y < image.GetHeight(); y++)
    {
      if (image.GetAlpha(x, y) > 0x13) // ~11% opacity
      {
        image.SetRGB(x, y,
                     std::clamp(R, (unsigned char) 0, image.GetRed(x, y)),
                     std::clamp(G, (unsigned char) 0, image.GetRed(x, y)),
                     std::clamp(B, (unsigned char) 0, image.GetRed(x, y)));

        image.SetAlpha(x, y,
                       std::clamp(A, (unsigned char) 0, image.GetAlpha(x, y)));
      }      
    }  
  }

  wxImage coloured_reticle = reticle;
  for (int x = 0; x < coloured_reticle.GetWidth(); x++)
  {
    for (int y = 0; y < coloured_reticle.GetHeight(); y++)
    {
      coloured_reticle.SetRGB(x, y, 0x0, 0xFF, 0x0);
    }
  }

  dc.DrawBitmap(background, 0, 0, false);
  dc.DrawBitmap(*new wxBitmap(image), 0, 0, false);
  dc.DrawBitmap(*new wxBitmap(coloured_reticle),
                (background.GetWidth() / 2) - (coloured_reticle.GetWidth() / 2),
                (background.GetHeight() / 2) - (coloured_reticle.GetHeight() / 2));
  
  event.Skip();
}

void PreviewWidget::OnEraseBackground(wxEraseEvent& event) {
  event.Skip();
}
