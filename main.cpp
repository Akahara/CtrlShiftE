#include <iostream>

#include <Windows.h>
#include "imgui/imgui.h"

#define CSE_EXPOSE_INTERNALS
#include "cse.h"

#include "extensions/debug_window.h"

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ PSTR lpCmdLine, _In_ INT nCmdShow)
{
  // initialization
  cse::graphics::loadGraphics();
  cse::keys::loadResources();
  cse::extensions::loadDefaultExtensions();
  cse::keys::registerGlobalHook();
  cse::graphics::createWindow(std::make_shared<cse::extensions::DebugWindowProcess>());

  // main loop
  while (!cse::graphics::shouldDispose()) {
    cse::graphics::render();
    cse::keys::pollEvents();
    cse::extensions::runLoopUpdates();
    cse::extensions::runDelayedTasks();
  }

  // finalization
  cse::extensions::unloadExtensions();
  cse::keys::disposeHookAndResources();
  cse::graphics::destroyGraphics();

  return 0;
}
