#include "startup_commands.h"

#include <fstream>
#include <regex>
#include <unordered_set>

#include "cse_commands.h"

namespace cse::extensions
{

StartupCommands::StartupCommands()
{
  executeCommandSet(readStartupCommandsFromFile(getConfigFile(), false));
}

void StartupCommands::reload()
{
  executeCommandSet(readStartupCommandsFromFile(getConfigFile(), true));
}

fs::path StartupCommands::getConfigFile()
{
  return cse::extensions::getUserConfigFilePath("startup.txt",
    "[cse] time window\n");
}

std::vector<StartupCommands::Runnable> StartupCommands::readStartupCommandsFromFile(const fs::path& path, bool onlyReloadCommands)
{
  std::vector<Runnable> runnables;

  std::ifstream is{ path };
  for(size_t l = 0; is; l++)
  {
    std::string line;
    std::getline(is, line);

    auto logLineError = [&](auto &&...err)
    {
      cse::logm("Invalid line ", l, ":'", line, "', ", std::forward<decltype(err)>(err)...);
    };

    if (line.starts_with("#") || line.empty())
      continue;

    std::unordered_set<std::string> flags;

    std::regex re{ R"(\[([^\]]+)\]\s+)" };
    for (std::smatch match; std::regex_search(line, match, re); )
    {
      flags.insert(match[1].str());
      line = match.suffix();
    }

    if (onlyReloadCommands && !flags.erase("reload"))
      continue;

    if(flags.erase("cse"))
    {
      runnables.emplace_back([line=std::move(line)] {
        cse::logm("Executing CSE ", line);
        if (!cse::commands::executeCommand(line))
          cse::logm("Could not execute command \"", line, '"');
      });
    }
    else if(flags.erase("cmd"))
    {
      runnables.emplace_back([line=std::move(line)]() mutable {
        cse::extensions::runDetached([line = std::move(line)] {
          cse::extensions::executeShellCommand(line.c_str());
        });
      });
    }
    else
    {
      logLineError("Unknown command type");
    }

    for(auto &unusedFlag : flags)
      logLineError("Unknown startup command flag(s) specified: ", unusedFlag);
  }

  return runnables;
}

void StartupCommands::executeCommandSet(std::vector<Runnable> commands)
{
  std::ranges::for_each(commands, [](auto &cmd) { cmd(); });
}

}
