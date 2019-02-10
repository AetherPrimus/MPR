#pragma once
#include "InputCommon/ControllerInterface/DInput/DInput8.h"
#include "wxWidgets3\include\wx\event.h"

namespace InputExternal
{

  // initialize the underlying mouse input data
  void InitMouse(IDirectInput8* const idi8);

  /* DInputMouse -
  Retrieves mouse input using DirectInput8, intended use outside of standard controller interface
  Input data updated synchronously through UpdateInput method
  Input data retrieved though GetDeltaAxis
  */
  class DInputMouse
  {
  private:
    LPDIRECTINPUTDEVICE8 m_mo_device;

    // accumulated deltas (these should be reset very periodically)
    LONG dx, dy;

  public:
    DInputMouse();
    // Initialize this class with a device
    void Init(LPDIRECTINPUTDEVICE8 mo_device);

    // Update mouse movement deltas relative to last call to UpdateInput
    void UpdateInput();

    // INTERNAL: On mouse left-click
    void OnWindowClick(wxMouseEvent& ev);

    // Locks the mouse cursor to the center of the RenderFrame
    void LockCursorToGameWindow();

    // Resets accumulated input information
    void ResetDeltas();

    // Retrieve horizontal and vertical mouse movement since last frame
    LONG GetDeltaVerticalAxis();
    LONG GetDeltaHorizontalAxis();

  private:
    DIMOUSESTATE2 state_prev;
    ULONGLONG last_update;
  };

  extern DInputMouse g_mouse_input;
}
