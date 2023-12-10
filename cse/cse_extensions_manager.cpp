
#define CSE_EXPOSE_INTERNALS
#include "cse_graphics.h"
#include "cse_commands.h"
#undef CSE_EXPOSE_INTERNALS

#include "extensions/key_stats_recorder.h"
#include "extensions/time_recorder.h"
#include "extensions/universal_shortcut.h"
#include "extensions/color_picker.h"
#include "extensions/tables.h"
#include "extensions/gotodir.h"
#include "extensions/keyswaps.h"
#include "extensions/numbers.h"
#include "extensions/regex.h"
#include "extensions/yt_dl.h"
#include "extensions/startup_commands.h"
#include "extensions/expressions/expressions.h"

namespace cse::extensions
{

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

void loadDefaultExtensions()
{
  loadExtension<cse::extensions::UniversalShortcut>();
  loadExtension<cse::extensions::TimeRecorder>();
  loadExtension<cse::extensions::KeyStatsRecorder>();
  loadExtension<cse::extensions::ColorPicker>();
  loadExtension<cse::extensions::Tables>();
  loadExtension<cse::extensions::GotoDir>();
  loadExtension<cse::extensions::KeySwaps>();
  loadExtension<cse::extensions::Numbers>();
  loadExtension<cse::extensions::YtDl>();
  loadExtension<cse::extensions::Regex>();
  loadExtension<cse::extensions::StartupCommands>();

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

void runLoopUpdates()
{
  for (auto &extension : s_activeExtensions)
    extension->update();
}

void unloadExtensions()
{
  s_activeExtensions.clear();
  cse::commands::getCommands().clear();
}

}