// TODO: move this into a folder
#include "DolphinWX/HackConfigDiag.h"

#include <wx/app.h>
#include <wx/sizer.h>

#include "Common/IniFile.h"
#include "Core/HackConfig.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"
#include "InputCommon/ControlReference/ControlReference.h"

static std::string RemoveTrailingZero(std::string const& str)
{
  std::string ret = str;
  int i = ret.length() - 1;
  while (i > 0 && (ret[i] == '0'))
  {
    ret.erase(ret.begin() + i);
    i--;
  }
  if (ret[i] == '.')
  {
    ret.erase(ret.begin() + i);
  }
  return ret;
}

static std::string FormatLabelString(std::string const& str)
{
  std::string ret = str;
  for (int i = 0; i < ret.length(); i++)
  {
    if (ret[i] == '&')
    {
      ret.insert(ret.begin() + i, '&');
      i++;
    }
  }
  return ret;
}

class TextInputDialog final : public wxDialog
{
private:
  std::string* expression_string;
  wxTextCtrl* text_ctrl;
  void OnOk(wxCommandEvent& event)
  {
    *expression_string = text_ctrl->GetValue().ToStdString();
    this->Destroy();
  }

  void OnCancel(wxCommandEvent& event)
  {
    this->Destroy();
  }

public:
  TextInputDialog(wxWindow* parent, std::string* expr) : wxDialog(parent, wxID_ANY,
    "Custom Expression", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
  {
    const int space5 = FromDIP(5);
    const int space3 = FromDIP(3);

    expression_string = expr;
    text_ctrl = new wxTextCtrl(this, wxID_ANY, *expr,
      wxDefaultPosition, wxDLG_UNIT(this, wxSize(100, 32)),
      wxTE_MULTILINE | wxTE_RICH2);
    {
      auto f = text_ctrl->GetFont();
      f.SetFamily(wxFONTFAMILY_MODERN);
      text_ctrl->SetFont(f);
    }
    auto* const enter_button = new wxButton(this, wxID_ANY, "OK");
    auto* const cancel_button = new wxButton(this, wxID_ANY, "Cancel");
    auto* const szr_main = new wxBoxSizer(wxVERTICAL);
    szr_main->AddSpacer(space5);
    szr_main->Add(text_ctrl, 0, wxEXPAND | wxLEFT | wxRIGHT, space5);
    szr_main->AddSpacer(space3);
    szr_main->Add(enter_button, 0, wxEXPAND | wxLEFT | wxRIGHT, space5);
    szr_main->AddSpacer(space3);
    szr_main->Add(cancel_button, 0, wxEXPAND | wxLEFT | wxRIGHT, space5);
    szr_main->AddSpacer(space5);

    enter_button->Bind(wxEVT_BUTTON, &TextInputDialog::OnOk, this);
    cancel_button->Bind(wxEVT_BUTTON, &TextInputDialog::OnCancel, this);
    
    SetSizerAndFit(szr_main);
    Center();
  }
};

wxBoxSizer* HackConfigDialog::CreateInputButton(std::string const& label,
  std::string const& name, std::string const& text)
{
  const int space5 = FromDIP(5);
  const int space3 = FromDIP(3);
  auto* const grid = new wxFlexGridSizer(2, 0, space3);
  auto* const button_box = new wxBoxSizer(wxHORIZONTAL);
  auto* const button_label = new wxStaticText(this, wxID_ANY, label);
  auto* const button = new wxButton(this, wxID_ANY, FormatLabelString(text),
    wxDefaultPosition, wxSize(88, 26));
  grid->Add(button_label, 0, wxALIGN_CENTER_VERTICAL, wxALIGN_RIGHT);
  grid->Add(button, 0, wxALIGN_CENTER_VERTICAL);
  button_box->Add(grid, 0, wxEXPAND | wxLEFT | wxRIGHT, space5);
  button_label->SetName(label);

  {
    auto f = button->GetFont();
    f.SetPointSize(8);
    button->SetFont(f);
  }

  button->Bind(wxEVT_BUTTON, &HackConfigDialog::DetectControl, this);
  button->Bind(wxEVT_RIGHT_UP, &HackConfigDialog::CreateExpression, this);
  button->Bind(wxEVT_MIDDLE_DOWN, &HackConfigDialog::ClearExpression, this);
  button->SetName(name);
  button->SetToolTip("Left-click to detect input.\nMiddle-click to clear.\nRight-click for more options.");

  return button_box;
}

void HackConfigDialog::CreateExpression(wxEvent& event)
{
  auto& controls = prime::GetMutableControls();
  wxButton* btn = (wxButton*)event.GetEventObject();
  int index = btn->GetName()[0] - '1';
  if (index > 7 || index < 0) return;
  // warning: dangerous:
  std::string expr = controls[index]->GetExpression();
  TextInputDialog tid = TextInputDialog(this, &expr);
  tid.ShowModal();
  btn->SetLabel(FormatLabelString(expr));
  controls[index]->SetExpression(expr);
}

void HackConfigDialog::ClearExpression(wxEvent& event)
{
  auto& controls = prime::GetMutableControls();
  wxButton* btn = (wxButton*)event.GetEventObject();
  int index = btn->GetName()[0] - '1';
  if (index > 7 || index < 0) return;
  controls[index]->SetExpression("");
  btn->SetLabel("");
}

// ~~~~PASTE~~~~~

inline bool IsAlphabetic(wxString& str)
{
  for (wxUniChar c : str)
    if (!isalpha(c))
      return false;

  return true;
}

inline void GetExpressionForControl(wxString& expr, wxString& control_name,
  const ciface::Core::DeviceQualifier* control_device = nullptr,
  const ciface::Core::DeviceQualifier* default_device = nullptr)
{
  expr = "";

  // non-default device
  if (control_device && default_device && !(*control_device == *default_device))
  {
    expr += control_device->ToString();
    expr += ":";
  }

  // append the control name
  expr += control_name;

  if (!IsAlphabetic(expr))
    expr = wxString::Format("`%s`", expr);
}

bool HackConfigDialog::DetectButton(wxButton* button, ControlReference* ref)
{
  bool success = false;
  const auto dev = g_controller_interface.FindDevice(
      ciface::Core::DeviceQualifier("DInput", 0, "Keyboard Mouse"));

  if (dev != nullptr)
  {
    std::string old_name = button->GetLabel().ToStdString();
    button->SetLabel(_("[ waiting ]"));

    wxTheApp->Yield(true);

    ciface::Core::Device::Control* const ctrl = ref->Detect(4000, dev.get());
    if (ctrl)
    {
      wxString control_name = ctrl->GetName();
      wxString expr;
      GetExpressionForControl(expr, control_name);
      button->SetLabel(FormatLabelString(expr.ToStdString()));
      ref->SetExpression(expr.ToStdString());
      success = true;
    }
    else
    {
      button->SetLabel(old_name);
    }
    wxTheApp->Yield(true);
  }
  
  return success;
}

// ~~~~END PASTE~~~~~

void HackConfigDialog::DetectControl(wxCommandEvent& event)
{
  auto& controls = prime::GetMutableControls();
  wxButton* btn = (wxButton*)event.GetEventObject();
  auto sz = btn->GetSize();
  int index = btn->GetName()[0] - '1';
  if (index > 7 || index < 0) return;
  // warning: dangerous:
  DetectButton(btn, controls[index].get());
}

void HackConfigDialog::OnSensitivitySliderChanged(wxCommandEvent& event)
{
  wxSlider* slider = (wxSlider*)event.GetEventObject();
  prime::SetSensitivity(slider->GetValue());
  sensitivity_box->SetValue(
    RemoveTrailingZero(std::to_string(prime::GetSensitivity())));
}

void HackConfigDialog::OnCursorSensitivitySliderChanged(wxCommandEvent& event)
{
  wxSlider* slider = (wxSlider*)event.GetEventObject();
  prime::SetCursorSensitivity(slider->GetValue());
  cursor_sensitivity_box->SetValue(
    RemoveTrailingZero(std::to_string(prime::GetCursorSensitivity())));
}

void HackConfigDialog::OnOk(wxCommandEvent& event)
{
  do_save = true;
  this->Destroy();
}
void HackConfigDialog::OnCancel(wxCommandEvent& event)
{
  do_save = false;
  this->Destroy();
}

void HackConfigDialog::OnEnter(wxKeyEvent& event)
{
  try
  {
    float sens = std::stof(sensitivity_box->GetValue().ToStdString());
    prime::SetSensitivity(sens);
  }
  catch (...)
  {
    event.Skip();
    return;
  }

  sensitivity_slider->SetValue(std::round(prime::GetSensitivity()));
  event.Skip();
}

void HackConfigDialog::OnEnter2(wxKeyEvent& event)
{
  try
  {
    float sens = std::stof(cursor_sensitivity_box->GetValue().ToStdString());
    prime::SetCursorSensitivity(sens);
  }
  catch (...)
  {
    event.Skip();
    return;
  }

  cursor_sensitivity_slider->SetValue(std::round(prime::GetSensitivity()));
  event.Skip();
}

HackConfigDialog::HackConfigDialog(wxWindow* const parent)
    : wxDialog(parent, wxID_ANY, "Hack Settings", wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE)
{
  float sensitivity = prime::GetSensitivity();
  float cursor_sensitivity = prime::GetCursorSensitivity();
  auto& controls = prime::GetMutableControls();

  const int space5 = FromDIP(5);
  const int space3 = FromDIP(3);

  auto* const sensitivity_label = new wxStaticText(this, wxID_ANY, "Camera Sensitivity");
  auto* const cursor_sensitivity_label = new wxStaticText(this, wxID_ANY, "Cursor Sensitivity");
  auto* const sensitivity_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "");
  sensitivity_slider = new wxSlider(this, wxID_ANY, std::round(sensitivity),
    1, 100, wxDefaultPosition);
  sensitivity_box = new wxTextCtrl(this, wxID_ANY,
    RemoveTrailingZero(std::to_string(sensitivity)));
  cursor_sensitivity_slider = new wxSlider(this, wxID_ANY, std::round(cursor_sensitivity),
    1, 100, wxDefaultPosition);
  cursor_sensitivity_box = new wxTextCtrl(this, wxID_ANY,
    RemoveTrailingZero(std::to_string(cursor_sensitivity)));

  sensitivity_sizer->Add(sensitivity_label, 0, wxEXPAND | wxLEFT | wxRIGHT);
  sensitivity_sizer->AddSpacer(space3);
  sensitivity_sizer->Add(sensitivity_slider, 0, wxEXPAND | wxLEFT | wxRIGHT);
  sensitivity_sizer->AddSpacer(space3);
  sensitivity_sizer->Add(sensitivity_box, 0, wxEXPAND | wxLEFT | wxRIGHT, space3);
  sensitivity_sizer->AddSpacer(space3);
  sensitivity_sizer->Add(cursor_sensitivity_label, 0, wxEXPAND | wxLEFT | wxRIGHT);
  sensitivity_sizer->AddSpacer(space3);
  sensitivity_sizer->Add(cursor_sensitivity_slider, 0, wxEXPAND | wxLEFT | wxRIGHT);
  sensitivity_sizer->AddSpacer(space3);
  sensitivity_sizer->Add(cursor_sensitivity_box, 0, wxEXPAND | wxLEFT | wxRIGHT, space3);

  auto* const buttons_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "Buttons");
  auto* const beam_sizer = new wxBoxSizer(wxVERTICAL);
  beam_sizer->Add(CreateInputButton("Beam 1", "1", controls[0]->GetExpression()));
  beam_sizer->AddSpacer(space3);
  beam_sizer->Add(CreateInputButton("Beam 2", "2", controls[1]->GetExpression()));
  beam_sizer->AddSpacer(space3);
  beam_sizer->Add(CreateInputButton("Beam 3", "3", controls[2]->GetExpression()));
  beam_sizer->AddSpacer(space3);
  beam_sizer->Add(CreateInputButton("Beam 4", "4", controls[3]->GetExpression()));
  auto* const visor_sizer = new wxBoxSizer(wxVERTICAL);
  visor_sizer->Add(CreateInputButton("Visor 1", "5", controls[4]->GetExpression()));
  visor_sizer->AddSpacer(space3);
  visor_sizer->Add(CreateInputButton("Visor 2", "6", controls[5]->GetExpression()));
  visor_sizer->AddSpacer(space3);
  visor_sizer->Add(CreateInputButton("Visor 3", "7", controls[6]->GetExpression()));
  visor_sizer->AddSpacer(space3);
  visor_sizer->Add(CreateInputButton("Visor 4", "8", controls[7]->GetExpression()));
  buttons_sizer->Add(beam_sizer);
  buttons_sizer->AddSpacer(space5);
  buttons_sizer->Add(visor_sizer);


  auto* const enter_button = new wxButton(this, wxID_ANY, "OK");
  auto* const cancel_button = new wxButton(this, wxID_ANY, "Cancel");
  auto* const szr_dlgbuttons = new wxBoxSizer(wxHORIZONTAL);
  szr_dlgbuttons->Add(enter_button);
  szr_dlgbuttons->AddSpacer(space3);
  szr_dlgbuttons->Add(cancel_button);

  auto* const szr_main = new wxBoxSizer(wxVERTICAL);
  szr_main->AddSpacer(space5);
  szr_main->Add(sensitivity_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, space5);
  szr_main->AddSpacer(space5);
  szr_main->Add(buttons_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, space5);
  szr_main->AddSpacer(space5);
  szr_main->Add(szr_dlgbuttons, 0, wxALIGN_CENTER, space3);
  szr_main->AddSpacer(space5);

  sensitivity_slider->Bind(wxEVT_SLIDER, &HackConfigDialog::OnSensitivitySliderChanged,
    this);
  cursor_sensitivity_slider->Bind(wxEVT_SLIDER, &HackConfigDialog::OnCursorSensitivitySliderChanged,
    this);
  enter_button->Bind(wxEVT_BUTTON, &HackConfigDialog::OnOk, this);
  cancel_button->Bind(wxEVT_BUTTON, &HackConfigDialog::OnCancel, this);
  sensitivity_box->Bind(wxEVT_KEY_UP, &HackConfigDialog::OnEnter, this);
  cursor_sensitivity_box->Bind(wxEVT_KEY_UP, &HackConfigDialog::OnEnter2, this);

  SetSizerAndFit(szr_main);
  Center();
}

HackConfigDialog::~HackConfigDialog()
{
  if(do_save) prime::SaveStateToIni();
  prime::LoadStateFromIni();
  prime::RefreshControlDevices();
}
