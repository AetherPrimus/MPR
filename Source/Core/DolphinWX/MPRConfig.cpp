#include "MPRConfig.h"

#include "DolphinWX/WxUtils.h"

#include <wx/debug.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/gbsizer.h>
#include <wx/statbmp.h>
#include <wx/clrpicker.h>
#include <wx/generic/statbmpg.h>
#include <wx/msgdlg.h>
#include <wx/statbox.h>
#include <wx/checklst.h>

#include "Core/ConfigManager.h"
#include "Core/HW/Wiimote.h"
#include "Core/HW/GCPad.h"
#include "Core/HW/WiimoteReal/WiimoteReal.h"
#include "Core/HW/SI/SI.h"
#include "Core/PrimeHack/TextureSwapper.h"
#include "Core/Core.h"

#include "Common/FileUtil.h"
#include "Common/CommonPaths.h"

#include "InputCommon/ControllerEmu/ControllerEmu.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"
#include "InputCommon/ControllerInterface/Device.h"
#include "InputCommon/InputConfig.h"

MPRConfig::MPRConfig(wxWindow* parent, wxWindowID id, const wxString& title,
  const wxPoint& position, const wxSize& size, long style)
  : wxDialog(parent, id, title, position, size, style)
{
  m_event_on_close = wxEVT_NULL;

  Bind(wxEVT_CLOSE_WINDOW, &MPRConfig::OnClose, this);
  Bind(wxEVT_BUTTON, &MPRConfig::OnCloseButton, this, wxID_CLOSE);
  Bind(wxEVT_SHOW, &MPRConfig::OnShow, this);

  wxDialog::SetExtraStyle(GetExtraStyle() & ~wxWS_EX_BLOCK_EVENTS);

  CreateGUIControls();
  BindEvents();
  LoadGUIValues();
}

MPRConfig::~MPRConfig()
{
}

void MPRConfig::SetSelectedTab(wxWindowID tab_id)
{
  switch (tab_id)
  {
  case ID_CONTROLS:
  case ID_HUD:
  case ID_DLC:
    Notebook->SetSelection(Notebook->FindPage(Notebook->FindWindowById(tab_id)));
    break;

  default:
    wxASSERT_MSG(false, wxString::Format("Invalid tab page ID specified (%d)", tab_id));
    break;
  }
}

void MPRConfig::CreateGUIControls()
{
  Notebook = new wxNotebook(this, ID_NOTEBOOK);

  wxPanel* const controls_pane = CreateControlsTab();
  wxPanel* const hud_pane = CreateHUDTab();
  wxPanel* const dlc_pane = CreateDLCTab();

  Notebook->AddPage(controls_pane, _("Control Presets"));
  Notebook->AddPage(hud_pane, _("HUD and Reticles"));
  Notebook->AddPage(dlc_pane, _("Additional Content"));

  const int space5 = FromDIP(5);
  const int space10 = FromDIP(10);

  wxBoxSizer* const main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->AddSpacer(space10);
  main_sizer->Add(Notebook, 1, wxEXPAND | wxLEFT | wxRIGHT, space5);
  main_sizer->AddSpacer(space5);
  main_sizer->Add(CreateButtonSizer(wxCLOSE), 0, wxEXPAND | wxLEFT | wxRIGHT, space5);
  main_sizer->AddSpacer(space5);

#ifdef __APPLE__
  main_sizer->SetMinSize(550, 0);
#else
  main_sizer->SetMinSize(FromDIP(400), 0);
#endif

  SetLayoutAdaptationMode(wxDIALOG_ADAPTATION_MODE_ENABLED);
  SetLayoutAdaptationLevel(wxDIALOG_ADAPTATION_STANDARD_SIZER);
  SetSizerAndFit(main_sizer);
}


wxPanel* MPRConfig::CreateControlsTab()
{
  const int space5 = FromDIP(5);
  const int space10 = FromDIP(10);
  wxPanel* panel = new wxPanel(Notebook, ID_PRESETS);
  wxStaticBoxSizer* const main_sizer =
      new wxStaticBoxSizer(wxVERTICAL, panel, _("Control Profiles"));

  m_ctrl_presets = new wxListView(panel);
  m_ctrl_presets->SetSingleStyle(wxLC_SINGLE_SEL);
  m_ctrl_presets->AppendColumn("Default Profiles");
  m_ctrl_presets->InsertItem(MouseKeyboard, _("Mouse & Keyboard"));
  m_ctrl_presets->InsertItem(DualStickXBX, _("DualStick Xbox One (XInput)"));
  m_ctrl_presets->InsertItem(DualStickSwitch, _("DualStick Switch Pro (XInput/BetterJoy)"));
  m_ctrl_presets->InsertItem(NativeHardware, _("Native (Real Wii Remote/GC Adapter)"));
  m_ctrl_presets->SetColumnWidth(0, 300);

  main_sizer->AddSpacer(space5);
  main_sizer->Add(m_ctrl_presets, 0, wxALL | wxEXPAND, space5);
  main_sizer->AddSpacer(space5);

  wxBoxSizer* const apply_sizer = new wxBoxSizer(wxHORIZONTAL);
  m_apply_preset = new wxButton(panel, wxID_ANY, "Apply Profile");
  apply_sizer->AddStretchSpacer(2);
  apply_sizer->AddSpacer(space5);
  apply_sizer->Add(m_apply_preset);
  apply_sizer->AddSpacer(space5);

  main_sizer->Add(apply_sizer);
  main_sizer->AddStretchSpacer();

  m_apply_preset->Bind(wxEVT_BUTTON, &MPRConfig::OnApplyCtrlPreset, this);

  panel->SetSizerAndFit(main_sizer);

  return panel;
}

wxPanel* MPRConfig::CreateHUDTab()
{
  const int space5 = FromDIP(5);
  const int space10 = FromDIP(10);
  wxPanel* panel = new wxPanel(Notebook, ID_HUD);

  HUD_Notebook = new wxNotebook(panel, ID_NOTEBOOK);

  wxStaticBoxSizer* const preview_sizer =
      new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Preview"));

  preview_widget = new PreviewWidget(panel);

  preview_sizer->Add(preview_widget);

  wxPanel* const hud_pane = CreateHUDColoursPage();
  wxPanel* const presets_pane = CreatePresetsPage();
  wxPanel* const reticles_pane = CreateReticlesPage();
  HUD_Notebook->AddPage(presets_pane, _("Preset Profiles"));
  HUD_Notebook->AddPage(hud_pane, _("HUD Tweaks"));
  HUD_Notebook->AddPage(reticles_pane, _("Reticle Options"));

  wxBoxSizer* const main_sizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer* const sizer = new wxBoxSizer(wxHORIZONTAL);

  sizer->AddSpacer(space5);
  sizer->Add(HUD_Notebook, 0, wxLEFT | wxEXPAND | wxBOTTOM);
  sizer->AddSpacer(space10);
  sizer->Add(preview_sizer, 0, wxEXPAND | wxRIGHT, space5);
  sizer->AddStretchSpacer();

  main_sizer->AddSpacer(space5);
  main_sizer->Add(sizer);
  main_sizer->AddSpacer(space5);
  main_sizer->AddStretchSpacer();

  HUD_Notebook->Layout();

  panel->SetSizerAndFit(main_sizer);

  return panel;
}

wxPanel* MPRConfig::CreateDLCTab()
{
  const int space5 = FromDIP(5);
  const int space10 = FromDIP(10);
  wxPanel* panel = new wxPanel(Notebook, ID_DLC);

  wxBoxSizer* const main_sizer = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer* const left_sizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer* const right_sizer = new wxBoxSizer(wxVERTICAL);

  m_dlc_list = new wxCheckListBox(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
  m_dlc_description = new wxStaticText();
  m_dlc_description->SetMinSize(wxSize(-200, -1));
  m_dlc_description->SetLabelText("Select an option for more information!");

  left_sizer->AddSpacer(space5);
  left_sizer->Add(m_dlc_list, 0, wxALL | wxEXPAND, space5);
  left_sizer->AddStretchSpacer();
  left_sizer->Add(m_dlc_description, 0, wxALL | wxEXPAND, space5);
  left_sizer->AddSpacer(space5);

  m_dlc_preview = new wxStaticBitmap();

  right_sizer->AddSpacer(space5);
  right_sizer->Add(m_dlc_preview, 0, wxALL | wxEXPAND, space5);
  right_sizer->AddSpacer(space5);

  main_sizer->AddSpacer(space5);
  main_sizer->Add(left_sizer, 0, wxALL | wxEXPAND, space5);
  main_sizer->AddSpacer(space5);
  main_sizer->Add(right_sizer, 0, wxALL | wxEXPAND, space5);
  main_sizer->AddSpacer(space5);

  m_hud_presets->Bind(wxEVT_CHECKLISTBOX, &MPRConfig::OnDLCChanged, this);

  panel->SetSizerAndFit(main_sizer);

  return panel;
}

wxPanel* MPRConfig::CreatePresetsPage()
{
  const int space5 = FromDIP(5);
  const int space10 = FromDIP(10);
  wxPanel* panel = new wxPanel(HUD_Notebook, ID_PRESETS);
  
  wxBoxSizer* const main_sizer = new wxBoxSizer(wxVERTICAL);
  m_hud_presets = new wxListView(panel);
  m_hud_presets->SetSingleStyle(wxLC_SINGLE_SEL);

  m_hud_presets->AppendColumn("Hud Presets");
  m_hud_presets->InsertItem(Default, _("Default Hud"));
  m_hud_presets->InsertItem(MinimalMode, _("Default (Minimal Mode)"));
  m_hud_presets->InsertItem(Hunters, _("Hunters Style"));
  m_hud_presets->InsertItem(HuntersMinimal, _("Hunters Style (Minimal Mode)"));
  m_hud_presets->InsertItem(Red, _("Red"));
  m_hud_presets->InsertItem(RedMinimal, _("Red (Minimal Style)"));
  m_hud_presets->InsertItem(Custom, _("Custom"));
  m_hud_presets->SetColumnWidth(0, 300);

  m_preset_warning_sizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Warning");
  m_preset_warning_sizer->Add(
      new wxStaticText(panel, wxID_ANY,
                       "Please note that not all elements of the HUD update in real time. To fully "
                       "update the HUD, you must enter a save-station."),
      0, wxALL | wxEXPAND, space5);
  m_preset_warning_sizer->AddStretchSpacer();

  main_sizer->AddSpacer(space5);
  main_sizer->Add(m_hud_presets, 0, wxALL | wxEXPAND, space5);
  main_sizer->AddStretchSpacer();
  main_sizer->Add(m_preset_warning_sizer, 0, wxLEFT | wxRIGHT, space5);
  main_sizer->AddSpacer(space5);

  m_hud_presets->Bind(wxEVT_LIST_ITEM_SELECTED, &MPRConfig::OnHUDPresetSelected, this);

  panel->SetSizerAndFit(main_sizer);

  return panel;
}

wxPanel* MPRConfig::CreateHUDColoursPage()
{
  const int space5 = FromDIP(5);
  const int space10 = FromDIP(10);
  wxPanel* panel = new wxPanel(HUD_Notebook, ID_HUDCOLOURS);
  wxBoxSizer* const main_sizer = new wxBoxSizer(wxVERTICAL);

  m_primary_hud_colour =
      new wxColourPickerCtrl(panel, wxID_ANY, GetHudColourValue(), wxDefaultPosition, wxDefaultSize,
                             wxCLRP_DEFAULT_STYLE, wxDefaultValidator, _("Change"));

  m_reset_btn = new wxButton(panel, wxID_ANY, _("Reset Colours"));

  m_hud_opacity =
      new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, *new wxSize(50, -1), wxSP_ARROW_KEYS,
                     0, 0xFF, SConfig::GetInstance().m_mpr_primary_hudcolour & 0x000000FF);

  m_hud_zoom =
      new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, *new wxSize(50, -1), wxSP_ARROW_KEYS,
                     -20, 20, SConfig::GetInstance().m_mpr_hud_zoom_factor);

  m_minimal_mode = new wxCheckBox(panel, wxID_ANY, _("Enable Minimal Combat HUD Mode"));
  m_minimal_mode->SetValue(SConfig::GetInstance().m_mpr_minimal_mode);

  wxGridBagSizer* settings_grid = new wxGridBagSizer(space10, space10);
  settings_grid->Add(new wxStaticText(panel, wxID_ANY, _("Primary Hud Colour")),
    wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
  settings_grid->Add(m_primary_hud_colour, wxGBPosition(0, 1), wxDefaultSpan,
                     wxALIGN_CENTER_VERTICAL);
  settings_grid->Add(m_reset_btn, wxGBPosition(0, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

  settings_grid->Add(new wxStaticText(panel, wxID_ANY, _("Hud Opacity")),
    wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
  settings_grid->Add(m_hud_opacity, wxGBPosition(1, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

  
  settings_grid->Add(new wxStaticText(panel, wxID_ANY, _("Hud Zoom Factor")), wxGBPosition(2, 0),
                     wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
  settings_grid->Add(m_hud_zoom, wxGBPosition(2, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

  m_custom_warning_sizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Warning");
  m_custom_warning_sizer->Add(
      new wxStaticText(panel, wxID_ANY,
                       "Please note that not all elements of the HUD update in real time. To fully "
                       "update the HUD, you must enter a save-station."),
      0, wxALL | wxEXPAND, space5);
  m_custom_warning_sizer->AddStretchSpacer();
    
  main_sizer->AddSpacer(space10);
  main_sizer->Add(settings_grid, 0, wxLEFT | wxRIGHT, space5);
  main_sizer->AddSpacer(space10);
  main_sizer->Add(m_minimal_mode, 0, wxLEFT | wxRIGHT, space5);
  main_sizer->AddStretchSpacer();
  main_sizer->Add(m_custom_warning_sizer, 0, wxLEFT | wxRIGHT, space5);
  main_sizer->AddSpacer(space5);

  panel->SetSizerAndFit(main_sizer);

  return panel;
}

wxPanel* MPRConfig::CreateReticlesPage()
{
  const int space5 = FromDIP(5);
  const int space10 = FromDIP(10);
  wxPanel* panel = new wxPanel(HUD_Notebook, ID_PRESETS);
  wxBoxSizer* const main_sizer =
      new wxBoxSizer(wxVERTICAL);

  m_reticle_selection = new wxListView(panel);
  m_reticle_selection->SetSingleStyle(wxLC_SINGLE_SEL);

  m_reticle_selection->AppendColumn("Reticle Options");
  m_reticle_selection->InsertItem(prime::MPR, _("MPR's Dynamic Reticles"));
  m_reticle_selection->InsertItem(prime::Dot, _("Dot Reticle"));
  m_reticle_selection->InsertItem(prime::Standard, _("Standard"));
  m_reticle_selection->InsertItem(prime::None, _("Empty / None"));
  m_reticle_selection->SetColumnWidth(0, 300);

  main_sizer->AddSpacer(space5);
  main_sizer->Add(m_reticle_selection, 0, wxALL | wxEXPAND, space5);
  main_sizer->AddSpacer(space5);
  main_sizer->AddStretchSpacer();

  m_reticle_selection->Bind(wxEVT_LIST_ITEM_SELECTED, &MPRConfig::OnReticleSelected, this);

  panel->SetSizerAndFit(main_sizer);

  return panel;
}
 
void MPRConfig::BindEvents()
{
  m_primary_hud_colour->Bind(wxEVT_COLOURPICKER_CHANGED, &MPRConfig::OnColourPressed, this);
  m_minimal_mode->Bind(wxEVT_CHECKBOX, &MPRConfig::OnMinimalToggled, this);
  m_hud_opacity->Bind(wxEVT_SPINCTRL, &MPRConfig::OnOpacityChanged, this);
  m_hud_zoom->Bind(wxEVT_SPINCTRL, &MPRConfig::OnZoomChanged, this);
  m_reset_btn->Bind(wxEVT_BUTTON, &MPRConfig::OnResetPressed, this);
}

void MPRConfig::OnMinimalToggled(wxCommandEvent& event)
{
  SConfig::GetInstance().m_mpr_minimal_mode = event.IsChecked(); 
  SConfig::GetInstance().SaveSettings();

  preview_widget->SetUseMinimal(event.IsChecked());
  preview_widget->UpdatePreview();

  SelectItemNoNotify(m_hud_presets, Custom);
}

void MPRConfig::OnColourPressed(wxCommandEvent& event)
{
  wxColour c = m_primary_hud_colour->GetColour();

  // BLUE-GREEN-RED
  u32 rgba = c.GetRGBA();
  u32 r = (rgba & 0xFF000000);
  u32 g = (rgba & 0x00FF0000);
  u32 b = (rgba & 0x0000FF00);
  u32 a = (rgba & 0x000000FF);
  c.SetRGBA((b | g | r | (a & 0xFE))); // Leave 1 bit for reset

  m_primary_hud_colour->SetColour(c);

  SaveGUIValues();

  SelectItemNoNotify(m_hud_presets, Custom);
}

void MPRConfig::OnResetPressed(wxCommandEvent& event) {
  m_primary_hud_colour->SetColour(*new wxColour(0xFFDFAB5E)); // 0x5EABDFFF
  m_hud_opacity->SetValue(0xFF);
  m_minimal_mode->SetValue(false);
  m_hud_zoom->SetValue(1);

  SaveGUIValues();

  SelectItemNoNotify(m_hud_presets, Default); 
}

void MPRConfig::OnHUDPresetSelected(wxCommandEvent& event)
{
  m_primary_hud_colour->SetColour(*new wxColour(0xFFDFAB5E)); // 0x5EABDFFF
  m_hud_opacity->SetValue(0xFF);
  m_minimal_mode->SetValue(false);

  switch (m_hud_presets->GetFirstSelected())
  {
    case MinimalMode:
      m_minimal_mode->SetValue(true);
      break;

    case HuntersMinimal:
      m_minimal_mode->SetValue(true); 
    case Hunters:
      m_primary_hud_colour->SetColour(RGB(52, 164, 0));
      m_hud_opacity->SetValue(235);

      break;

    case RedMinimal:
      m_minimal_mode->SetValue(true); 
    case Red:
      m_primary_hud_colour->SetColour(RGB(165, 0, 0));

      break;

    default:
      break;
  }

  SaveGUIValues();
}

void MPRConfig::OnApplyCtrlPreset(wxCommandEvent& event)
{
  int answer = wxMessageBox(wxT("Are you sure you want to apply this profile? This will overwrite any unsaved "
                   "active profiles."),
               wxT("Are you sure?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this);

  if (answer == wxNO)
    return;

  if (m_ctrl_presets->GetFirstSelected() == (long) NativeHardware)
  {
    WiimoteReal::ChangeWiimoteSource(0, 2);

    SConfig::GetInstance().m_SIDevice[0] = SerialInterface::SIDEVICE_WIIU_ADAPTER;

    if (Core::IsRunning())
      SerialInterface::ChangeDevice(SerialInterface::SIDEVICE_WIIU_ADAPTER, 0);

    return;
  }
  else {
    WiimoteReal::ChangeWiimoteSource(0, 1);

    SConfig::GetInstance().m_SIDevice[0] = SerialInterface::SIDEVICE_GC_CONTROLLER;

    if (Core::IsRunning())
      SerialInterface::ChangeDevice(SerialInterface::SIDEVICE_GC_CONTROLLER, 0);
  }

  if (m_ctrl_presets->GetFirstSelected() == (long) MouseKeyboard)
  {
    Wiimote::GetConfig()->GetController(0)->LoadDefaults(g_controller_interface);
    Wiimote::GetConfig()->GetController(0)->UpdateReferences(g_controller_interface);
    Wiimote::GetConfig()->SaveConfig();

    Pad::GetConfig()->GetController(0)->LoadDefaults(g_controller_interface);
    Pad::GetConfig()->GetController(0)->UpdateReferences(g_controller_interface);
    Pad::GetConfig()->SaveConfig();

    return;
  }

  char* wii_path = new char[MAX_PATH];
  sprintf(wii_path, "%s/AetherLabs/defaults/wiimote/%d.ini", File::GetSysDirectory().c_str(),
          m_ctrl_presets->GetFirstSelected());

  IniFile inifile;
  if (inifile.Load(wii_path))
  {
    Wiimote::GetConfig()->GetController(0)->LoadConfig(inifile.GetOrCreateSection("Profile"));
    Wiimote::GetConfig()->GetController(0)->UpdateReferences(g_controller_interface);
  }

  char* gc_path = new char[MAX_PATH];
  sprintf(gc_path, "%s/AetherLabs/defaults/gamecube/%d.ini", File::GetSysDirectory().c_str(),
          m_ctrl_presets->GetFirstSelected());

  if (inifile.Load(gc_path))
  {
    Pad::GetConfig()->GetController(0)->LoadConfig(inifile.GetOrCreateSection("Profile"));
    Pad::GetConfig()->GetController(0)->UpdateReferences(g_controller_interface);
  }
}

void MPRConfig::OnReticleSelected(wxCommandEvent& event)
{
  prime::SetReticle((prime::ReticleSelection) m_reticle_selection->GetFirstSelected());
  preview_widget->SetReticleImage((prime::ReticleSelection) m_reticle_selection->GetFirstSelected());

  SaveGUIValues();
}

void MPRConfig::OnZoomChanged(wxCommandEvent& event)
{
  SaveGUIValues();

  m_reset_btn->Enable();
}

void MPRConfig::OnDLCChanged(wxCommandEvent& event)
{

}

void MPRConfig::OnOpacityChanged(wxCommandEvent& event)
{
  SaveGUIValues();

  SelectItemNoNotify(m_hud_presets, Custom);
  m_reset_btn->Enable();
}

wxColour MPRConfig::GetHudColourValue()
{
  wxColour* colour = new wxColour();

  // ALHPA-BLUE-GREEN-RED
  u32 rgba = SConfig::GetInstance().m_mpr_primary_hudcolour;
  u32 r = (rgba & 0xFF000000) >> 24;
  u32 g = (rgba & 0x00FF0000) >> 8;
  u32 b = (rgba & 0x0000FF00) << 8;
  u32 a = (rgba & 0x000000FF) << 24;
  colour->SetRGBA(a | b | g | r);

  return *colour;
}

void MPRConfig::LoadGUIValues()
{
  m_primary_hud_colour->SetBackgroundColour(GetHudColourValue());
  m_hud_opacity->SetValue(SConfig::GetInstance().m_mpr_primary_hudcolour & 0x000000FF);
  m_minimal_mode->SetValue(SConfig::GetInstance().m_mpr_minimal_mode);
  m_hud_zoom->SetValue(SConfig::GetInstance().m_mpr_hud_zoom_factor);

  wxColour c = m_primary_hud_colour->GetColour();
  unsigned char r = c.Red();
  unsigned char g = c.Green();
  unsigned char b = c.Blue();
  unsigned char a = c.Alpha();

  preview_widget->SetRGBA(r, g, b, a - 0x50 > a ? 0 : a - 0x50);
  preview_widget->SetUseMinimal(SConfig::GetInstance().m_mpr_minimal_mode);
  preview_widget->SetReticleImage((prime::ReticleSelection) SConfig::GetInstance().m_mpr_reticle_selection);
  preview_widget->UpdatePreview();

  if (SConfig::GetInstance().m_mpr_primary_hudcolour == 0x94171223FF)
    m_reset_btn->Disable();
}

void MPRConfig::SaveGUIValues() {
  SConfig& settings = SConfig::GetInstance();

  wxColour primary = m_primary_hud_colour->GetColour();
  unsigned char r = primary.Red();
  unsigned char g = primary.Green();
  unsigned char b = primary.Blue();
  unsigned char a = static_cast<unsigned char>(m_hud_opacity->GetValue());

  settings.m_mpr_primary_hudcolour = (r << 24 | g << 16 | b << 8 | a);
  settings.m_mpr_minimal_mode = m_minimal_mode->GetValue();
  settings.m_mpr_reticle_selection =
      (prime::ReticleSelection) m_reticle_selection->GetFirstSelected();
  settings.m_mpr_hud_zoom_factor = m_hud_zoom->GetValue();

  settings.SaveSettings();

  m_reset_btn->Enable(settings.m_mpr_primary_hudcolour != -1);

  preview_widget->SetUseMinimal(settings.m_mpr_minimal_mode);
  preview_widget->SetRGBA(r, g, b, a - 0x50 > a ? 0 : a - 0x50);
  preview_widget->UpdatePreview();

  prime::SetReticle((prime::ReticleSelection)m_reticle_selection->GetFirstSelected());
}

void MPRConfig::OnClose(wxCloseEvent& WXUNUSED(event))
{
  Hide();
  SaveGUIValues();

  if (m_event_on_close != wxEVT_NULL)
    AddPendingEvent(wxCommandEvent{ m_event_on_close });
}

void MPRConfig::OnShow(wxShowEvent& event)
{
  if (event.IsShown())
    CenterOnParent();

  m_custom_warning_sizer->GetStaticBox()->Show(Core::IsRunning());
  m_preset_warning_sizer->GetStaticBox()->Show(Core::IsRunning());

  m_custom_warning_sizer->GetItem((size_t) 0)->Show(Core::IsRunning());
  m_preset_warning_sizer->GetItem((size_t) 0)->Show(Core::IsRunning());
}

void MPRConfig::OnCloseButton(wxCommandEvent& WXUNUSED(event))
{
  Close();
}
