#include "startup_commands.h"

#include <fstream>
#include <regex>
#include <unordered_set>

#include "cse_commands.h"

namespace cse::extensions
{

StartupCommands::StartupCommands()
{
  FileCommands commands = readCommandsFromFile(getConfigFile());
  reloadCSECommands(std::move(commands.cseCommands));
  executeCommandSet(commands.startupCommands);
}

void StartupCommands::reload()
{
  FileCommands commands = readCommandsFromFile(getConfigFile());
  reloadCSECommands(std::move(commands.cseCommands));
  executeCommandSet(std::move(commands.reloadCommands));
}

void StartupCommands::reloadCSECommands(std::vector<CSECommandPrototype> &&newCommands)
{
  for (auto &loadedCommand : m_loadedCseCommands)
    cse::commands::removeCommand(loadedCommand.name);
  m_loadedCseCommands.clear();

  for(auto &newCommand : newCommands) {
    if (cse::commands::hasCommand(newCommand.name)) {
      cse::logm("CSE command ", newCommand.name, " already exists and cannot be replaced!");
      continue;
    }
    
    cse::commands::addCommand({
      newCommand.name,
      newCommand.tooltip,
      { /*no arguments*/ },
      [runnable=newCommand.runnable](auto &args) { runnable(); }
    });
    m_loadedCseCommands.push_back(std::move(newCommand));
  }
}

fs::path StartupCommands::getConfigFile()
{
  return cse::extensions::getUserConfigFilePath("startup.txt",
    "# You can put commands here, pass flags to dictate when and how the\n"
    "# commands will be executed (see the examples and the app code to see\n"
    "# exactly what's possible)\n"
    "[type=cse] time window\n"
    "[type=shell] [command=python] [command-tooltip=python shell] [shell-interactive] C:\\Windows\\System32\\cmd.exe /c python\n"
    "[type=url] [command=regex] [command-tooltip=Regex101 substitution and matching] https://regex101.com/\n");
}

StartupCommands::FileCommands StartupCommands::readCommandsFromFile(const fs::path& path)
{
  FileCommands commands;

  std::ifstream is{ path };
  for(size_t l = 0; is; l++) {
    std::string line;
    std::getline(is, line);

    if (line.starts_with("#") || line.empty())
      continue;

    std::unordered_map<std::string, std::string> flags;

    std::regex re{ R"(\[([^\]=]+)(?:=([^\]]+))?\]\s+)" };
    for (std::smatch match; std::regex_search(line, match, re); ) {
      flags.emplace(match[1].str(), match[2].str());
      line = match.suffix();
    }

    auto logLineError = [&](auto &&...err) {
      cse::logm("Invalid line ", l, ":'", line, "', ", std::forward<decltype(err)>(err)...);
    };
    auto consumeFlag = [&](const char *name) -> std::pair<bool, std::string> {
      if (!flags.contains(name))
        return { false, "" };
      std::pair p{ true, flags[name] };
      flags.erase(name);
      return p;
    };

    Runnable commandRunnable;

    if (!flags.contains("type")) {
      logLineError("Missing type");
      continue;
    }

    std::string eventType = consumeFlag("type").second;

    if(eventType == "cse") {
      commandRunnable = [line=std::move(line)] {
        cse::logm("Executing CSE ", line);
        if (!cse::commands::executeCommand(line))
          cse::logm("Could not execute command \"", line, '"');
      };
    } else if(eventType == "shell") {
      bool interactive = consumeFlag("shell-interactive").first;
      commandRunnable = [line=std::move(line), interactive]() mutable {
        cse::extensions::runDetached([&] {
          cse::extensions::executeShellCommand(line, interactive);
        });
      };
    } else if (eventType == "url") {
      commandRunnable = [line = std::move(line)]() mutable {
        cse::extensions::runDetached([&] {
          cse::extensions::openWebPage(line.c_str());
        });
      };
    } else {
      logLineError("Unknown command runnable type");
      continue;
    }

    bool interpreted = false;

    if (auto [hasCommandFlag, commandFlag] = consumeFlag("command"); hasCommandFlag) {
      interpreted = true;
      std::string commandName = commandFlag;
      std::string tooltip = consumeFlag("command-tooltip").second;
      commands.cseCommands.push_back({ commandName, tooltip, commandRunnable });
    }
    if (auto [hasStartupFlag, startupFlag] = consumeFlag("startup"); hasStartupFlag) {
      interpreted = true;
      commands.startupCommands.push_back(commandRunnable);
      commands.reloadCommands.push_back(commandRunnable);
    }
    if (auto [hasReloadFlag, reloadFlag] = consumeFlag("reload"); hasReloadFlag) {
      interpreted = true;
      commands.reloadCommands.push_back(commandRunnable);
    }

    if (!interpreted)
      logLineError("No event specification flag");

    for(auto &unusedFlag : flags)
      logLineError("Unknown command flag(s) specified: ", unusedFlag.first, "=", unusedFlag.second);
  }

  return commands;
}

void StartupCommands::executeCommandSet(std::vector<Runnable> commands)
{
  std::ranges::for_each(commands, [](auto &cmd) { cmd(); });
}

}
