#include "gotodir.h"

#include <fstream>
#include <filesystem>
#include <Windows.h>

namespace cse::extensions {

fs::path GotoDir::getSettingsFile()
{
  return cse::extensions::getUserConfigFilePath("cd.txt", (
    "# You can put directory shortcuts here\n"
    "# for example the following line adds a shortcut named 'foo' to the 'C:\\foo\\bar directory\n"
    "# foo C:\\foo\\bar\n"
    "# if 'bar' is a file its parent directory will be opened instead\n"
    "# when editing this file run 'reload' from CSE to apply changes\n"
    "cd " + (cse::extensions::getUserFilesPath() / "cd.txt").string()).c_str());
}

void GotoDir::loadDirPaths()
{
  std::ifstream file{ getSettingsFile() };
  char lineBuf[256];
  while (file.good()) {
    file.getline(lineBuf, sizeof(lineBuf));
    std::string line{ lineBuf };
    size_t split = line.find(' ');
    if (split == 0 || split == std::string::npos || line[0] == '#')
      continue;
    std::string dirname = line.substr(0, split);
    std::string dirpath = line.substr(split + 1);
    auto existingIdx = std::ranges::find(m_dirnames, dirname);
    if (existingIdx != m_dirnames.end()) {
      m_dirpaths[existingIdx - m_dirnames.begin()] = dirpath; // replace existing
    } else {
      m_dirnames.push_back(dirname);
      m_dirpaths.emplace_back(dirpath);
    }
  }
}

void GotoDir::addDirPath(std::string_view name, std::string_view path)
{
  std::ofstream file{ getSettingsFile(), std::ios_base::app };
  file << name << ' ' << path << std::endl;
}

GotoDir::GotoDir()
{
  loadDirPaths();

  m_directoryNameInput = std::make_shared<CommandEnumPart>("dirname", m_dirnames);
  
  cse::commands::addCommand({
    "cd",
    "open a directory shortcut",
    { m_directoryNameInput },
    [this](const auto &args) {
      auto idx = std::find(m_dirnames.begin(), m_dirnames.end(), args[0]);
      assert(idx != m_dirnames.end());
      fs::path path = m_dirpaths[idx - m_dirnames.begin()];
      if (fs::is_regular_file(path))
        path = path.parent_path();
      cse::extensions::openFileDir(path.string().c_str());
    }
  });
  
  cse::commands::addCommand({
    "cmd",
    "open a shell shortcut",
    { m_directoryNameInput },
    [this](const auto &args) {
      auto idx = std::find(m_dirnames.begin(), m_dirnames.end(), args[0]);
      assert(idx != m_dirnames.end());
      fs::path path = m_dirpaths[idx - m_dirnames.begin()];
      if (fs::is_regular_file(path))
        path = path.parent_path();
      if (!ShellExecuteA(NULL, "open", "cmd.exe", NULL, path.string().c_str(), SW_SHOWNORMAL))
        cse::logm("Could not open a shell at ", path);
    }
  });

  cse::commands::addCommand({
    "cdadd",
    "add a directory shortcut",
    { std::make_shared<CommandTextPart>("dirname"), std::make_shared<CommandTextPart>("path") },
    [this](const auto &args) {
      const std::string_view &dirname = args[0];
      const std::string_view &dirpath = args[1];
      addDirPath(dirname, dirpath);
      reload();
    }
  });
}

void GotoDir::reload()
{
  m_dirnames.clear();
  m_dirpaths.clear();
  loadDirPaths();
  m_directoryNameInput->setParts(m_dirnames);
}

}