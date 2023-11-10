#include <iostream>

#include <Windows.h>
#include "imgui/imgui.h"

#define CSE_EXPOSE_INTERNALS
#include "cse.h"

#include "extensions/debug_window.h"
#include "extensions/key_stats_recorder.h"
#include "extensions/time_recorder.h"
#include "extensions/universal_shortcut.h"
#include "extensions/color_picker.h"
#include "extensions/tables.h"
#include "extensions/gotodir.h"
#include "extensions/keyswaps.h"
#include "extensions/numbers.h"
#include "extensions/startup_commands.h"
#include "extensions/expressions/expressions.h"

static std::vector<std::unique_ptr<CSEExtension>> s_activeExtensions;

static void reloadExtensions()
{
  std::ranges::for_each(s_activeExtensions, [](auto &ext) { ext->reload(); });
}

template<class Ext> requires std::derived_from<Ext, CSEExtension>
static void loadExtension()
{
  cse::logm("Loading extension ", Ext::EXTENSION_NAME);
  s_activeExtensions.emplace_back(std::make_unique<Ext>());
}

static void loadDefaultExtensions()
{
  loadExtension<cse::extensions::UniversalShortcut>();
  loadExtension<cse::extensions::TimeRecorder>();
  loadExtension<cse::extensions::KeyStatsRecorder>();
  loadExtension<cse::extensions::ColorPicker>();
  loadExtension<cse::extensions::Tables>();
  loadExtension<cse::extensions::GotoDir>();
  loadExtension<cse::extensions::KeySwaps>();
  loadExtension<cse::extensions::StartupCommands>();
  loadExtension<cse::extensions::Numbers>();

  cse::commands::addCommand({
    "reload",
    "reload extensions",
    { /* no parameters */ },
    [](const auto &parts) {
      cse::extensions::runLater([] {
        cse::log("Reloading extensions");
        reloadExtensions();
      });
    }
  });
  
  cse::commands::addCommand({
    "quit",
    "quit CtrlShiftE",
    { /* no parameters */ },
    [](const auto &parts) {
      cse::graphics::closeAllWindows();
    }
  });

  cse::commands::addCommand({
    "exit",
    "quit CtrlShiftE",
    { /* no parameters */ },
    [](const auto &parts) {
      cse::graphics::closeAllWindows();
    }
  });
}

static void unloadExtensions()
{
  s_activeExtensions.clear();
  cse::commands::getCommands().clear();
}

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ PSTR lpCmdLine, _In_ INT nCmdShow)
{
  cse::graphics::loadGraphics();
  cse::keys::loadResources();
  loadDefaultExtensions();
  cse::keys::registerGlobalHook();
  cse::graphics::createWindow(std::make_shared<cse::extensions::DebugWindowProcess>());

  while (!cse::graphics::shouldDispose()) {
    cse::graphics::render();
    cse::keys::pollEvents();
    cse::extensions::runDelayedTasks();
  }

  // finalization
  unloadExtensions();

  // post-finalization
  cse::keys::disposeHookAndResources();
  cse::graphics::destroyGraphics();

  return 0;
}