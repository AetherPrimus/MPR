#pragma once

#include <map>
#include <string>

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "InputCommon/ControlReference/ControlReference.h"
constexpr int input_wait_time = 4000;

class HackConfigDialog final : public wxDialog
{
private:
  bool do_save = true;
  wxSlider* sensitivity_slider;
  wxTextCtrl* sensitivity_box;
private:
  void OnSensitivitySliderChanged(wxCommandEvent&);
  void OnOk(wxCommandEvent&);
  void OnCancel(wxCommandEvent&);
  void OnEnter(wxKeyEvent&);

  void DetectControl(wxCommandEvent&);
  bool DetectButton(wxButton* button, ControlReference* ref);

  void CreateExpression(wxEvent&);
  void ClearExpression(wxEvent&);

  wxBoxSizer* CreateInputButton(std::string const& label, std::string const& name,
    std::string const& text);

public:
  HackConfigDialog(wxWindow* parent);
  ~HackConfigDialog();
};

/*
#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>

class SensitivitySelector : public wxDialog
{
public:
  SensitivitySelector(wxWindow* parent, wxWindowID id = wxID_ANY,
    const wxString& title = _("Sensitivity Slider"), const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE);

  void OnOK(wxCommandEvent&);

private:
  wxTextCtrl * m_text_entry;
};
*/
