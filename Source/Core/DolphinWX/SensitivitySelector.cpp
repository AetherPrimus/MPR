
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

#include "Common/IniFile.h"
#include "Core/ActionReplay.h"
#include "DolphinWX/SensitivitySelector.h"

SensitivitySelector::SensitivitySelector(wxWindow* parent, wxWindowID id, const wxString& title,
                                         const wxPoint& pos, const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style)
{
  wxBoxSizer* const sMain = new wxBoxSizer(wxVERTICAL);

  float sensitivity;
  {
    IniFile sens_file;
    sens_file.Load("config_sensitivity.ini", true);
    if (!sens_file.GetIfExists<float>("mouse", "PrimeHack_Sensitivity", &sensitivity))
    {
      auto* section = sens_file.GetOrCreateSection("mouse");
      section->Set("PrimeHack_Sensitivity", 7.5f);
      sensitivity = 7.5f;
      sens_file.Save("config_sensitivity.ini");
    }
  }

  m_text_entry = new wxTextCtrl(this, wxID_ANY, std::to_string(sensitivity));
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
