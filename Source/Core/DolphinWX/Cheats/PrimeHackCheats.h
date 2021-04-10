#pragma once

#include <wx/panel.h>

class wxCheckBox;

class PrimeHackCheats : public wxPanel
{
public:
  PrimeHackCheats(wxWindow* parent, wxWindowID id);

  void InitializeGUI();
  void LoadGUIValues();
  void BindEvents();

protected:
  void OnCheatsChanged(wxCommandEvent&);

private:
  wxCheckBox* m_checkbox_noclip;
  wxCheckBox* m_checkbox_invulnerability;
  wxCheckBox* m_checkbox_skipcutscenes;
  wxCheckBox* m_checkbox_scandash;
  wxCheckBox* m_checkbox_skipportalmp2;
  wxCheckBox* m_checkbox_friendvouchers;
  wxCheckBox* m_checkbox_hudmemo;
};
