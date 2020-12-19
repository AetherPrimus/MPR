#include "DolphinWX/CVarDlg.h"

#include <wx/listctrl.h>
#include <wx/textctrl.h>

#include "Core/PrimeHack/HackConfig.h"
#include "Core/PrimeHack/HackManager.h"
#include "Core/PrimeHack/Mods/ElfModLoader.h"

class CVarDlg;

namespace detail {
class SetCVarDlg : public wxDialog {
public:
  SetCVarDlg(CVarDlg* const parent, std::string const& varname, long row)
    : wxDialog(parent, wxID_ANY, "Set CVar", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE),
      set_cvar(varname), parent_window(parent), set_row(row) {
    wxStaticText* const varname_label = new wxStaticText(this, wxID_ANY, wxString::Format("Set value for CVar %s", varname.c_str()));
    input_val = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1));

    wxBoxSizer* const sizer = new wxBoxSizer(wxVERTICAL);
    const int space5 = FromDIP(5), space3 = FromDIP(3);
    sizer->AddSpacer(FromDIP(10));
    sizer->Add(varname_label, 0, wxLEFT | wxRIGHT, space5);
    sizer->AddSpacer(space5);
    sizer->Add(input_val, 0, wxCENTER, space5);
    sizer->AddSpacer(space5);
    sizer->Add(CreateButtonSizer(wxOK | wxCANCEL | wxNO_DEFAULT), 0, wxCENTER, space5);
    sizer->AddSpacer(FromDIP(10));

    Bind(wxEVT_BUTTON, &SetCVarDlg::PressOK, this, wxID_OK);

    SetSizerAndFit(sizer);
    SetFocus();
  }

private:
  wxTextCtrl *input_val;

  std::string set_cvar;
  CVarDlg *parent_window;
  long set_row;

  void PressOK(wxCommandEvent& ev) {
    parent_window->OnCVarSet(set_row, set_cvar, input_val->GetValue());
    ev.Skip();
  }
};
}

CVarDlg::CVarDlg(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos,
  const wxSize& size, long style)
  : wxDialog(parent, id, title, pos, size, style) {
  cvar_list = nullptr;
  empty_text = nullptr;
}

void CVarDlg::LoadControls() {
  prime::ElfModLoader *mod = static_cast<prime::ElfModLoader *>(prime::GetHackManager()->get_mod("elf_mod_loader"));
  std::vector<prime::CVar*> varlist;
  wxSizer* sizer;
  if (mod != nullptr) {
    mod->get_cvarlist(varlist);
  }

  if (varlist.empty()) {
    empty_text = new wxStaticText(this, wxID_ANY, "No mod loaded!");
    
    sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(empty_text, 0, wxALL, 30);
    SetSizerAndFit(sizer);
    return;
  }

  cvar_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);

  long c0 = cvar_list->AppendColumn("Name");
  long c1 = cvar_list->AppendColumn("Value");
  long c2 = cvar_list->AppendColumn("Type");

  int height = 13;
  for (prime::CVar* var : varlist) {
    long idx = cvar_list->InsertItem(0, var->name);
    switch (var->type) {
    case prime::CVarType::INT8: {
        u8 val;
        mod->get_cvar_val(var->name, &val, sizeof(u8));
        cvar_list->SetItem(idx, 1, wxString::Format("%hhd", val));
        cvar_list->SetItem(idx, 2, "int8");
      }
      break;
    case prime::CVarType::INT16: {
        u16 val;
        mod->get_cvar_val(var->name, &val, sizeof(u16));
        cvar_list->SetItem(idx, 1, wxString::Format("%hd", val));
        cvar_list->SetItem(idx, 2, "int16");
      }
      break;
    case prime::CVarType::INT32: {
        u32 val;
        mod->get_cvar_val(var->name, &val, sizeof(u32));
        cvar_list->SetItem(idx, 1, wxString::Format("%d", val));
        cvar_list->SetItem(idx, 2, "int32");
      }
      break;
    case prime::CVarType::INT64: {
        u64 val;
        mod->get_cvar_val(var->name, &val, sizeof(u64));
        cvar_list->SetItem(idx, 1, wxString::Format("%lld", val));
        cvar_list->SetItem(idx, 2, "int64");
      }
      break;
    case prime::CVarType::FLOAT32: {
        float val;
        mod->get_cvar_val(var->name, &val, sizeof(float));
        cvar_list->SetItem(idx, 1, wxString::Format("%f", val));
        cvar_list->SetItem(idx, 2, "float32");
      }
      break;
    case prime::CVarType::FLOAT64: {
        double val;
        mod->get_cvar_val(var->name, &val, sizeof(double));
        cvar_list->SetItem(idx, 1, wxString::Format("%f", val));
        cvar_list->SetItem(idx, 2, "float64");
      }
      break;
    default: {
        cvar_list->SetItem(idx, 1, "");
        cvar_list->SetItem(idx, 2, "unknown");
      }
      break;
    }

    wxRect rect;
    cvar_list->GetItemRect(idx, rect);
    height += rect.height;
  }

  cvar_list->SetColumnWidth(c0, wxLIST_AUTOSIZE);
  cvar_list->SetColumnWidth(c1, wxLIST_AUTOSIZE);
  cvar_list->SetColumnWidth(c2, wxLIST_AUTOSIZE);
  int width = cvar_list->GetColumnWidth(c0) + cvar_list->GetColumnWidth(c1) + cvar_list->GetColumnWidth(c2);
  cvar_list->SetClientSize(wxSize(width, height));
  cvar_list->SetMinClientSize(wxSize(width, height));

  cvar_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &CVarDlg::OnListViewItemActivated, this);

  sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(cvar_list, wxEXPAND);

  SetSizerAndFit(sizer);
}

void CVarDlg::ClearControls() {
  if (cvar_list) {
    cvar_list->Destroy();
    cvar_list = nullptr;
  }
  if (empty_text) {
    empty_text->Destroy();
    empty_text = nullptr;
  }
}

void CVarDlg::OnCVarSet(long idx, std::string const& var, wxString val) {
  prime::ElfModLoader *mod = static_cast<prime::ElfModLoader *>(prime::GetHackManager()->get_mod("elf_mod_loader"));
  auto* cvar = mod->get_cvar(var);
  if (cvar == nullptr) {
    return;
  }
  if (cvar->type == prime::CVarType::FLOAT32 || cvar->type == prime::CVarType::FLOAT64) {
    double d;
    if (!val.ToDouble(&d)) {
      return;
    }
    if (cvar->type == prime::CVarType::FLOAT32) {
      float f = static_cast<float>(d);
      mod->write_cvar(var, &f);
      cvar_list->SetItem(idx, 1, wxString::Format("%f", f));
    } else {
      mod->write_cvar(var, &d);
      cvar_list->SetItem(idx, 1, wxString::Format("%f", d));
    }
  } else {
    long long i;
    if (!val.ToLongLong(&i)) {
      return;
    }
    mod->write_cvar(var, &i);
    cvar_list->SetItem(idx, 1, val);
  }
}

void CVarDlg::OnListViewItemActivated(wxListEvent& evt) {
  long idx = evt.GetIndex();
  
  detail::SetCVarDlg cvar_dlg(this, cvar_list->GetItemText(idx, 0).ToStdString(), idx);
  cvar_dlg.ShowModal();
}
