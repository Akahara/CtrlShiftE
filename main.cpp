#include <iostream>

#include <Windows.h>
#include "imgui/imgui.h"

#include "cse/graphics.h"
#include "cse/cse.h"
#include "cse/cse_internal.h"

#include "cse/extensions/debug_window.h"
#include "cse/extensions/key_stats_recorder.h"
#include "cse/extensions/time_recorder.h"
#include "cse/extensions/universal_shortcut.h"
#include "cse/extensions/recorded_commands.h"
#include "cse/extensions/color_picker.h"
#include "cse/extensions/tables.h"
#include "cse/extensions/personnal_links.h"

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ PSTR lpCmdLine, _In_ INT nCmdShow)
{
  // pre-initialization
  graphics::loadGraphics();

  // initialization
  std::vector<cse::extensions::CSEExtension *> activeExtensions;
  activeExtensions.push_back(new cse::extensions::UniversalShortcut);
  activeExtensions.push_back(new cse::extensions::TimeRecorder);
  activeExtensions.push_back(new cse::extensions::KeyStatsRecorder);
  activeExtensions.push_back(new cse::extensions::RecordedCommands);
  activeExtensions.push_back(new cse::extensions::ColorPicker);
  activeExtensions.push_back(new cse::extensions::Tables);
  activeExtensions.push_back(new cse::extensions::PersonnalLinks);

  // post-initialization
  cse::keys::registerGlobalHook();

  graphics::createWindow(std::make_shared<DebugWindowProcess>());

  while (!graphics::shouldDispose()) {
    graphics::render();
    cse::keys::pollEvents();
  }

  // finalization
  for (auto *extension : activeExtensions)
    delete extension;
  activeExtensions.clear();

  // post-finalization
  cse::keys::unregisterGlobalHook();
  graphics::destroyGraphics();

  return 0;
}
