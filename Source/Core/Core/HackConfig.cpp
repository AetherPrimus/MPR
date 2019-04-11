#include "HackConfig.h"

#include <string>
#include <Windows.h>

#include "Common/IniFile.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"

namespace prime
{
  static std::vector<std::unique_ptr<ControlReference>> control_list;
  static float sensitivity;
  static const std::string config_path = "hack_config.ini";

  void InitializeHack()
  {
    IniFile cfg_file;
    cfg_file.Load(config_path, true);
    if (!cfg_file.GetIfExists<float>("mouse", "sensitivity", &sensitivity))
    {
      auto* section = cfg_file.GetOrCreateSection("mouse");
      section->Set("sensitivity", 7.5f);
      sensitivity = 7.5f;
    }

    control_list.resize(8);
    for (int i = 0; i < 4; i++)
    {
      std::string control = std::string("index_") + std::to_string(i);
      std::string control_expr;

      if (!cfg_file.GetIfExists<std::string>("beam", control.c_str(), &control_expr))
      {
        auto* section = cfg_file.GetOrCreateSection("beam");
        section->Set(control.c_str(), "");
        control_expr = "";
      }
      control_list[i] = std::make_unique<InputReference>();
      control_list[i]->UpdateReference(g_controller_interface,
        ciface::Core::DeviceQualifier("DInput", 0, "Keyboard Mouse"));
      control_list[i]->SetExpression(control_expr);

      if (!cfg_file.GetIfExists<std::string>("visor", control.c_str(), &control_expr))
      {
        auto* section = cfg_file.GetOrCreateSection("visor");
        section->Set(control.c_str(), "");
        control_expr = "";
      }
      control_list[i + 4] = std::make_unique<InputReference>();
      control_list[i + 4]->SetExpression(control_expr);
      control_list[i + 4]->UpdateReference(g_controller_interface,
        ciface::Core::DeviceQualifier("DInput", 0, "Keyboard Mouse"));
    }
    cfg_file.Save(config_path);
  }

  void RefreshControlDevices()
  {
    for (int i = 0; i < control_list.size(); i++)
    {
      control_list[i]->UpdateReference(g_controller_interface,
        ciface::Core::DeviceQualifier("DInput", 0, "Keyboard Mouse"));
    }
  }

  void SaveStateToIni()
  {
    IniFile cfg_file;
    cfg_file.Load(config_path, true);

    auto* sens_section = cfg_file.GetOrCreateSection("mouse");
    sens_section->Set("sensitivity", sensitivity);
    auto* beam_section = cfg_file.GetOrCreateSection("beam");
    auto* visor_section = cfg_file.GetOrCreateSection("visor");

    for (int i = 0; i < 4; i++)
    {
      std::string control = std::string("index_") + std::to_string(i);
      beam_section->Set(control, control_list[i]->GetExpression());
      visor_section->Set(control, control_list[i + 4]->GetExpression());
    }
    cfg_file.Save(config_path);
  }

  void LoadStateFromIni()
  {
    IniFile cfg_file;
    cfg_file.Load(config_path, true);

    auto* sens_section = cfg_file.GetOrCreateSection("mouse");
    sens_section->Get("sensitivity", &sensitivity);
    auto* beam_section = cfg_file.GetOrCreateSection("beam");
    auto* visor_section = cfg_file.GetOrCreateSection("visor");

    for (int i = 0; i < 4; i++)
    {
      std::string control = std::string("index_") + std::to_string(i);
      std::string expr;
      beam_section->Get(control, &expr);
      control_list[i]->SetExpression(expr);
      visor_section->Get(control, &expr);
      control_list[i + 4]->SetExpression(expr);
    }
    cfg_file.Save(config_path);
  }

  bool CheckBeamCtl(int beam_num)
  {
    return control_list[beam_num]->State() > 0.5;
  }

  bool CheckVisorCtl(int visor_num)
  {
    return control_list[visor_num + 4]->State() > 0.5;
  }

  std::vector<std::unique_ptr<ControlReference>>& GetMutableControls()
  {
    return control_list;
  }

  float GetSensitivity()
  {
    return sensitivity;
  }

  void SetSensitivity(float sens)
  {
    sensitivity = sens;
  }
}
