#include <iostream>

#include <Windows.h>
#include "imgui/imgui.h"

#define CSE_EXPOSE_INTERNALS
#include "cse.h"

#include "extensions/debug_window.h"
#include "extensions/universal_shortcut.h"

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ PSTR lpCmdLine, _In_ INT nCmdShow)
{
  std::string argv{ lpCmdLine };
  bool noKeys = argv.contains("--no-keys");

  std::filesystem::current_path(cse::extensions::getUserFilesPath());

  // initialization
  cse::graphics::loadGraphics();
  cse::keys::loadResources();
  cse::extensions::loadDefaultExtensions();
  cse::graphics::createWindow(std::make_shared<cse::extensions::DebugWindowProcess>());
  if (!noKeys) {
    cse::keys::registerGlobalHook();
  } else {
    cse::graphics::createWindow(std::make_shared<cse::extensions::UniversalShortcutWindow>(true));
  }

  // main loop
  while (!cse::graphics::shouldDispose()) {
    cse::graphics::render();
    if (!noKeys) cse::keys::pollEvents();
    cse::extensions::runLoopUpdates();
    cse::extensions::runDelayedTasks();
  }

  // finalization
  cse::extensions::unloadExtensions();
  if (!noKeys) cse::keys::disposeHookAndResources();
  cse::graphics::destroyGraphics();

  return 0;
}
