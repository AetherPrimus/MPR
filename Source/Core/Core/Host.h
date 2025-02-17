// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <string>

// Host - defines an interface for the emulator core to communicate back to the
// OS-specific layer

// The emulator core is abstracted from the OS using 2 interfaces:
// Common and Host.

// Common simply provides OS-neutral implementations of things like threads, mutexes,
// INI file manipulation, memory mapping, etc.

// Host is an abstract interface for communicating things back to the host. The emulator
// core is treated as a library, not as a main program, because it is far easier to
// write GUI interfaces that control things than to squash GUI into some model that wasn't
// designed for it.

// The host can be just a command line app that opens a window, or a full blown debugger
// interface.

class wxRect;
enum class MsgType;

bool wxMsgAlert(const char* caption, const char* text, bool yes_no, MsgType /*style*/);

bool Host_UINeedsControllerState();
bool Host_RendererHasFocus();
bool Host_RendererIsFullscreen();
void Host_RenderScreenToClient(int* x, int* y);
void Host_GetRendererClientRect(wxRect& rectOut);
void* Host_GetRenderFrame();
void Host_Message(int Id);
void Host_NotifyMapLoaded();
void Host_RefreshDSPDebuggerWindow();
void Host_RequestRenderWindowSize(int width, int height);
void Host_UpdateDisasmDialog();
void Host_UpdateMainFrame();
void Host_UpdateTitle(const std::string& title);
void Host_ShowVideoConfig(void* parent, const std::string& backend_name);
void Host_YieldToUI();
void Host_UpdateProgressDialog(const char* caption, int position, int total);

// TODO (neobrain): Remove this from host!
void* Host_GetRenderHandle();
