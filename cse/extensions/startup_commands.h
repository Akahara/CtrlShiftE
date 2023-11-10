#pragma once

#include "cse_extensions.h"

namespace cse::extensions
{

class StartupCommands : public CSEExtension
{
private:
  using Runnable = std::function<void()>;

  struct CSECommandPrototype {
    std::string name;
    std::string tooltip;
    Runnable runnable;
  };

  struct FileCommands {
    std::vector<Runnable> startupCommands;
    std::vector<Runnable> reloadCommands;
    std::vector<CSECommandPrototype> cseCommands;
  };

public:
  static constexpr const char *EXTENSION_NAME = "Startup Commands";

  StartupCommands();

  void reload() override;

private:
  std::vector<CSECommandPrototype> m_loadedCseCommands;

  void reloadCSECommands(std::vector<CSECommandPrototype> &&newCommands);

  static fs::path getConfigFile();
  static FileCommands readCommandsFromFile(const fs::path &path);
  static void executeCommandSet(std::vector<Runnable> commands);
};

}
