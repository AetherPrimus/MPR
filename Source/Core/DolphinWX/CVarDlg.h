#pragma once

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "Common/CommonTypes.h"

class CVarDlg : public wxDialog
{
public:
  explicit CVarDlg(wxWindow* parent, wxWindowID id = wxID_ANY,
                   const wxString& title = "CVars",
                   const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                   long style = wxDEFAULT_DIALOG_STYLE);

  void LoadControls();
  void ClearControls();
  void OnCVarSet(long idx, std::string const& var, wxString val);

private:
  wxListCtrl* cvar_list;
  wxStaticText* empty_text;

  void OnListViewItemActivated(wxListEvent&);
};
