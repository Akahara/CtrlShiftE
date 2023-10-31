#pragma once

#include "cse.h"

namespace cse::extensions {

class GotoDir : public CSEExtension {
public:
  static constexpr const char *EXTENSION_NAME = "Goto Dir";

  GotoDir();
  void reload() override;

private:
  static fs::path getSettingsFile();
  void loadDirPaths();
  void addDirPath(std::string_view name, std::string_view path);

  std::vector<std::string>         m_dirnames;
  std::vector<fs::path>            m_dirpaths;
  std::shared_ptr<CommandEnumPart> m_directoryNameInput;
};

}
