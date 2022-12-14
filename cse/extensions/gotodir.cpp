#include "gotodir.h"

#include <fstream>
#include <filesystem>
#include <assert.h>

namespace cse::extensions {

namespace fs = std::filesystem;

static std::vector<std::string> s_dirnames;
static std::vector<fs::path> s_dirpaths;
static CommandEnumPart *s_directoryNameInput;

static fs::path getSettingsFile()
{
  fs::path path = cse::getUserFilesPath() / "gotodir.txt";
  if (!fs::is_regular_file(path)) {
    std::ofstream file{ path };
    file 
      << "# You can put directory shortcuts here\n"
      << "# for example the following line adds a shortcut named 'foo' to the 'C:\\foo\\bar directory\n"
      << "# foo C:\\foo\\bar\n"
      << "# if 'bar' is a file its parent directory will be opened instead\n"
      << "# when editing this file run 'reload' from CSE to apply changes\n"
      << "gotodir " << path.string() << std::endl;
  }
  return path;
}

static void loadDirPaths()
{
  std::ifstream file{ getSettingsFile() };
  char lineBuf[256];
  while (file.good()) {
    file.getline(lineBuf, sizeof(lineBuf));
    std::string line{ lineBuf };
    size_t split = line.find(' ');
    if (split == 0 || split == std::string::npos || line[0] == '#')
      continue;
    s_dirnames.push_back(line.substr(0, split));
    s_dirpaths.push_back(line.substr(split+1));
  }
}

GotoDir::GotoDir()
{
  loadDirPaths();

  cse::addCommand({
    "gotodir",
    "open a directory shortcut",
    { s_directoryNameInput = new CommandEnumPart("dirname", s_dirnames) },
    [](const auto &args) {
      auto idx = std::find(s_dirnames.begin(), s_dirnames.end(), args[0]);
      assert(idx != s_dirnames.end());
      fs::path path = s_dirpaths[idx - s_dirnames.begin()];
      if (fs::is_regular_file(path))
        path = path.parent_path();
      cse::extensions::openFileDir(path.string().c_str());
    }
  });
}

GotoDir::~GotoDir()
{
}

void GotoDir::reload()
{
  s_dirnames.clear();
  s_dirpaths.clear();
  loadDirPaths();
  s_directoryNameInput->setParts(s_dirnames);
}

}