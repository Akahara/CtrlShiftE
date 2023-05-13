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
#include "cse/extensions/gotodir.h"

static std::vector<cse::extensions::CSEExtension *> s_activeExtensions;

static void reloadExtensions()
{
  for (cse::extensions::CSEExtension *ext : s_activeExtensions) {
    ext->reload();
  }
}

static void loadDefaultExtensions()
{
  s_activeExtensions.push_back(new cse::extensions::UniversalShortcut);
  s_activeExtensions.push_back(new cse::extensions::TimeRecorder);
  s_activeExtensions.push_back(new cse::extensions::KeyStatsRecorder);
  s_activeExtensions.push_back(new cse::extensions::RecordedCommands);
  s_activeExtensions.push_back(new cse::extensions::ColorPicker);
  s_activeExtensions.push_back(new cse::extensions::Tables);
  s_activeExtensions.push_back(new cse::extensions::GotoDir);

  cse::addCommand({
    "reload",
    "reload extensions",
    { /* no parameters */ },
    [](const auto &parts) {
      cse::log("Reloading extensions");
      reloadExtensions();
    }
  });
}

static void unloadExtensions()
{
  for (auto *extension : s_activeExtensions)
    delete extension;
  s_activeExtensions.clear();
  cse::getCommands().clear();
}

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ PSTR lpCmdLine, _In_ INT nCmdShow)
{
  // pre-initialization
  graphics::loadGraphics();

  // initialization
  loadDefaultExtensions();

  // post-initialization
  cse::keys::registerGlobalHook();

  graphics::createWindow(std::make_shared<DebugWindowProcess>());

  while (!graphics::shouldDispose()) {
    graphics::render();
    cse::keys::pollEvents();
  }

  // finalization
  unloadExtensions();

  // post-finalization
  cse::keys::unregisterGlobalHook();
  graphics::destroyGraphics();

  return 0;
}