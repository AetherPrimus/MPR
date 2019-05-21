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
  wxSlider* cursor_sensitivity_slider;
  wxTextCtrl* cursor_sensitivity_box;
  wxSlider* fov_slider;
  wxTextCtrl* fov_box;

private:
  void OnSensitivitySliderChanged(wxCommandEvent&);
  void OnCursorSensitivitySliderChanged(wxCommandEvent&);
  void OnFovSliderChanged(wxCommandEvent&);
  void OnOk(wxCommandEvent&);
  void OnCancel(wxCommandEvent&);
  void OnEnter(wxKeyEvent&);
  void OnEnter2(wxKeyEvent&);
  void OnEnter3(wxKeyEvent&);

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
