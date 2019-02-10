
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/button.h>

#include "DolphinWX/SensitivitySelector.h"
#include "Core/ActionReplay.h"

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
  Close();
}
