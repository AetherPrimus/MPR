
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/button.h>

#include "DolphinWX/SensitivitySelector.h"
#include "Core/ActionReplay.h"
#include "Common/IniFile.h"

SensitivitySelector::SensitivitySelector(wxWindow* parent, wxWindowID id,
  const wxString& title, const wxPoint& pos,
  const wxSize& size, long style)
  : wxDialog(parent, id, title, pos, size, style)
{
  wxBoxSizer* const sMain = new wxBoxSizer(wxVERTICAL);

  m_text_entry = new wxTextCtrl(this, wxID_ANY, "10");
  sMain->Add(m_text_entry);

  wxButton* const enterButton = new wxButton(this, wxID_ANY, _("Ok"));
  enterButton->Bind(wxEVT_BUTTON, &SensitivitySelector::OnOK, this);
  sMain->Add(enterButton);

  SetSizerAndFit(sMain);
  Center();
  SetFocus();
}

void SensitivitySelector::OnOK(wxCommandEvent& event)
{
  wxString val = m_text_entry->GetValue();
  double parsed_val;
  if (!val.ToDouble(&parsed_val))
  {
    Close();
    return;
  }

  ActionReplay::SetSensitivity(parsed_val);

  {
    IniFile sens_file;
    sens_file.Load("config_sensitivity.ini", true);
    sens_file.GetOrCreateSection("mouse")->Set("PrimeHack_Sensitivity", val.ToStdString());
    sens_file.Save("config_sensitivity.ini");
  }
  Close();
}
