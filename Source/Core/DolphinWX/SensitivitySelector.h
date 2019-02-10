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
