#pragma once

#include "cse_extensions.h"

namespace cse::extensions
{

class StartupCommands : public CSEExtension
{
public:
  static constexpr const char *EXTENSION_NAME = "Startup Commands";

  using Runnable = std::function<void()>;

  StartupCommands();

  void reload() override;

private:
  static fs::path getConfigFile();
  static std::vector<Runnable> readStartupCommandsFromFile(const fs::path &path, bool onlyReloadCommands);
  static void executeCommandSet(std::vector<Runnable> commands);
};

}
