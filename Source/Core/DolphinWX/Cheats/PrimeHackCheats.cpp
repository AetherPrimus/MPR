#include "DolphinWX/Cheats/PrimeHackCheats.h"

#include "DolphinWX/WxUtils.h"

#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/sizer.h>

#include "Core/ConfigManager.h"

PrimeHackCheats::PrimeHackCheats(wxWindow* parent, wxWindowID id) : wxPanel(parent, id)
{
  InitializeGUI();
  LoadGUIValues();
  BindEvents();
}

void PrimeHackCheats::InitializeGUI()
{
  m_checkbox_noclip = new wxCheckBox(this, wxID_ANY, _("Noclip"));
  m_checkbox_invulnerability = new wxCheckBox(this, wxID_ANY, _("Invulnerability"));
  m_checkbox_skipcutscenes = new wxCheckBox(this, wxID_ANY, _("Skippable Cutscenes"));
  m_checkbox_scandash = new wxCheckBox(this, wxID_ANY, _("Restore Scan Dash"));
  m_checkbox_skipportalmp2 = new wxCheckBox(this, wxID_ANY, _("Skip MP2 Portal Cutscene"));
  m_checkbox_friendvouchers = new wxCheckBox(this, wxID_ANY, _("Remove Friend Vouchers Requirement (Trilogy Only)"));
  m_checkbox_hudmemo = new wxCheckBox(this, wxID_ANY, _("Disable Hud Popup on Pickup Acquire"));

  m_checkbox_noclip->SetToolTip(_("Source Engine style noclip, fly through walls using the movement keys!"));
  m_checkbox_invulnerability->SetToolTip(_("Become resistant to all sources of damage. Projectiles literally will bounce off you!"));
  m_checkbox_skipcutscenes->SetToolTip(_("Make most cutscenes skippable. The button to do so varies from each game. It is usually the Jump key or the Menu button."));
  m_checkbox_scandash->SetToolTip(_("Re-enable the ability to dash with the scan visor. This is a speed-running trick in the original release, and was subsequently patched in later releases."));
  m_checkbox_skipportalmp2->SetToolTip(_("Skips having to watch the portal cutscenes in Metroid Prime 2 (Trilogy), allowing you to teleport immediately."));
  m_checkbox_friendvouchers->SetToolTip(_("Removes the friend voucher cost from all purchasable extras. This is on by default as friend-vouchers are impossible to obtain."));
  m_checkbox_hudmemo->SetToolTip(_("Removes the item pickup screen and explanation screen for powerups."));

  const int space5 = FromDIP(5);

  wxStaticBoxSizer* const cheats_sizer =
    new wxStaticBoxSizer(wxVERTICAL, this, _("Cheats"));
  cheats_sizer->AddSpacer(space5);
  cheats_sizer->Add(m_checkbox_noclip, 0, wxLEFT | wxRIGHT, space5);
  cheats_sizer->AddSpacer(space5);
  cheats_sizer->Add(m_checkbox_invulnerability, 0, wxLEFT | wxRIGHT, space5);
  cheats_sizer->AddSpacer(space5);
  cheats_sizer->Add(m_checkbox_skipcutscenes, 0, wxLEFT | wxRIGHT, space5);
  cheats_sizer->AddSpacer(space5);
  cheats_sizer->Add(m_checkbox_scandash, 0, wxLEFT | wxRIGHT, space5);
  cheats_sizer->AddSpacer(space5);
  cheats_sizer->Add(m_checkbox_skipportalmp2, 0, wxLEFT | wxRIGHT, space5);
  cheats_sizer->AddSpacer(space5);
  cheats_sizer->Add(m_checkbox_friendvouchers, 0, wxLEFT | wxRIGHT, space5);
  cheats_sizer->AddSpacer(space5);
  cheats_sizer->Add(m_checkbox_hudmemo, 0, wxLEFT | wxRIGHT, space5);
  cheats_sizer->AddSpacer(space5);

  wxBoxSizer* const main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->AddSpacer(space5);
  main_sizer->Add(cheats_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, space5);
  main_sizer->AddSpacer(space5);

  SetSizer(main_sizer);
}

void PrimeHackCheats::LoadGUIValues()
{
  const SConfig& config = SConfig::GetInstance();

  m_checkbox_noclip->SetValue(config.bPrimeNoclip);
  m_checkbox_invulnerability->SetValue(config.bPrimeInvulnerability);
  m_checkbox_skipcutscenes->SetValue(config.bPrimeSkipCutscene);
  m_checkbox_scandash->SetValue(config.bPrimeRestoreDashing);
  m_checkbox_skipportalmp2->SetValue(config.bPrimePortalSkip);
  m_checkbox_friendvouchers->SetValue(config.bPrimeFriendVouchers);
  m_checkbox_hudmemo->SetValue(config.bDisableHudMemoPopup);
}

void PrimeHackCheats::BindEvents()
{
  m_checkbox_noclip->Bind(wxEVT_CHECKBOX, &PrimeHackCheats::OnCheatsChanged, this);
  m_checkbox_invulnerability->Bind(wxEVT_CHECKBOX, &PrimeHackCheats::OnCheatsChanged, this);
  m_checkbox_skipcutscenes->Bind(wxEVT_CHECKBOX, &PrimeHackCheats::OnCheatsChanged, this);
  m_checkbox_scandash->Bind(wxEVT_CHECKBOX, &PrimeHackCheats::OnCheatsChanged, this);
  m_checkbox_skipportalmp2->Bind(wxEVT_CHECKBOX, &PrimeHackCheats::OnCheatsChanged, this);
  m_checkbox_friendvouchers->Bind(wxEVT_CHECKBOX, &PrimeHackCheats::OnCheatsChanged, this);
  m_checkbox_hudmemo->Bind(wxEVT_CHECKBOX, &PrimeHackCheats::OnCheatsChanged, this);
}

void PrimeHackCheats::OnCheatsChanged(wxCommandEvent&)
{
  SConfig& config = SConfig::GetInstance();

  config.bPrimeNoclip = m_checkbox_noclip->GetValue();
  config.bPrimeInvulnerability = m_checkbox_invulnerability->GetValue();
  config.bPrimeSkipCutscene = m_checkbox_skipcutscenes->GetValue();
  config.bPrimeRestoreDashing = m_checkbox_scandash->GetValue();
  config.bPrimePortalSkip = m_checkbox_skipportalmp2->GetValue();
  config.bPrimeFriendVouchers = m_checkbox_friendvouchers->GetValue();
  config.bDisableHudMemoPopup = m_checkbox_hudmemo->GetValue();

  config.SaveSettings();
}
