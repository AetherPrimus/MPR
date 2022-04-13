#pragma once

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <DolphinWX/MPRPreviewWidget.h>

class wxNotebook;
class wxPanel;
class wxButton;
class wxCheckBox;
class wxSpinCtrl;
class wxStaticText;
class wxColourPickerCtrl;
class wxStaticBoxSizer;
class wxGenericStaticBitmap;

class MPRConfig : public wxDialog
{
public:
  MPRConfig(wxWindow* parent, wxWindowID id = wxID_ANY,
    const wxString& title = _("MPR Configuration"),
    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
    long style = wxDEFAULT_DIALOG_STYLE);
  virtual ~MPRConfig();

  void SetSelectedTab(wxWindowID tab_id);

  enum
  {
    ID_NOTEBOOK = 1000,
    ID_PRESETS,
    ID_HUDCOLOURS,
    ID_RETICLES
  };

  enum
  {
    ID_HUDNOTEBOOK = 2000,
    ID_CONTROLS,
    ID_HUD,
  };

private:
  void CreateGUIControls();
  wxPanel* CreateControlsTab();
  wxPanel* CreateHUDTab();
  wxPanel* CreateHUDColoursPage();
  wxPanel* CreatePresetsPage();
  wxPanel* CreateReticlesPage();
 
  void BindEvents();

  void LoadGUIValues();
  void SaveGUIValues();
  void OnClose(wxCloseEvent& event);
  void OnCloseButton(wxCommandEvent& event);
  void OnShow(wxShowEvent& event);
  void OnMinimalToggled(wxCommandEvent& event);
  void OnColourPressed(wxCommandEvent& event);
  void OnOpacityChanged(wxCommandEvent& event);
  void OnResetPressed(wxCommandEvent& event);
  void OnHUDPresetSelected(wxCommandEvent& event);
  void OnApplyCtrlPreset(wxCommandEvent& event);
  void OnReticleSelected(wxCommandEvent& event);
  void OnZoomChanged(wxCommandEvent& event);

  inline void SelectItemNoNotify(wxListView* box, long item) {
    m_hud_presets->SetEvtHandlerEnabled(false);
    m_hud_presets->Select(item, true);
    m_hud_presets->SetEvtHandlerEnabled(true);
  }

  wxColour GetHudColourValue();

  wxNotebook* Notebook;
  wxEventType m_event_on_close;

  /* Preset Controls Tab */
  wxListView* m_ctrl_presets;
  wxButton* m_apply_preset;
  enum CtrlPresets : long
  {
    MouseKeyboard,
    DualStickPS4,
    DualStickXBX,
    DualStickSwitch,
    NativeHardware
  };

  /* HUD Tab */
  wxNotebook* HUD_Notebook;
  PreviewWidget* preview_widget;

  /* HUD - Presets */
  wxListView* m_hud_presets;
  wxStaticBoxSizer* m_preset_warning_sizer;
  enum HudPresets : long
  {
    Default,
    MinimalMode,
    Hunters,
    HuntersMinimal,
    Red,
    RedMinimal,
    Custom
  };

  /* HUD - Customise */
  wxColourPickerCtrl* m_primary_hud_colour;
  wxButton* m_reset_btn;
  wxSpinCtrl* m_hud_opacity;
  wxSpinCtrl* m_hud_zoom;
  wxStaticText* m_opacity_label;
  wxCheckBox* m_minimal_mode;
  wxStaticBoxSizer* m_custom_warning_sizer;

  /* HUD - Reticles */
  wxListView* m_reticle_selection;
};
