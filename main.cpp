#include <iostream>

#include "imgui/imgui.h"

#include "cse/graphics.h"
#include "cse/cse.h"
#include "cse/cse_internal.h"

#include "cse/extensions/debug_window.h"
#include "cse/extensions/key_stats_recorder.h"
#include "cse/extensions/time_recorder.h"
#include "cse/extensions/universal_shortcuts.h"
#include "cse/extensions/recorded_commands.h"

int main()
{
  graphics::loadGraphics();

  cse::keys::addGlobalySuppressedKeystroke(UniversalShortcutBringer::KEYSTROKE);
  cse::keys::registerGlobalHook();
  cse::keys::addGlobalKeyListener(std::make_shared<UniversalShortcutBringer>());
  cse::keys::addGlobalKeyListener(std::make_shared<KeyStatsRecorder>());
  cse::addIOTGenerator(std::make_shared<IOTTimeRecorder>());
  cse::addIOTGenerator(std::make_shared<IOTRecordedCommands>());

  graphics::createWindow(std::make_shared<DebugWindowProcess>());
  graphics::createWindow(std::make_shared<UniversalShortcutWindow>());

  while (!graphics::shouldDispose()) {
    graphics::render();
    cse::keys::pollEvents();
  }

  cse::keys::unregisterGlobalHook();
  cse::clearIOTGenerators();
  graphics::destroyGraphics();

  return 0;
}
