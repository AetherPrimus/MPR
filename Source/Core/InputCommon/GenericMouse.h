#pragma once

#include "wxWidgets3\include\wx\event.h"

namespace prime
{

class GenericMouse
{
public:
  // Platform dependant implementations are made virtual
  virtual void UpdateInput() = 0;
  virtual void LockCursorToGameWindow() = 0;

  void OnWindowClick(wxMouseEvent& ev);
  void ResetDeltas();
  int32_t GetDeltaVerticalAxis() const;
  int32_t GetDeltaHorizontalAxis() const;

protected:
  bool cursor_locked;
  int32_t dx, dy;
};

extern GenericMouse* g_mouse_input;

}  // namespace prime
