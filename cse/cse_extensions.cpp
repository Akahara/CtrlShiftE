#define CSE_EXPOSE_INTERNALS
#include "cse_extensions.h"
#undef CSE_EXPOSE_INTERNALS

#include <fstream>
#include <Windows.h>

namespace cse::extensions {

static const fs::path USER_FILES_DIR = ".CtrlShiftE";
static std::vector<std::function<void()>> s_delayedTasks;

const fs::path &getUserFilesPath()
{
  static fs::path path = []{
    char *home;
    size_t homeLength;
    _dupenv_s(&home, &homeLength, "USERPROFILE");
    if (home == nullptr)
      throw std::exception("Could not retrieve user's home");
    fs::path path = home / USER_FILES_DIR;
    if (!fs::is_directory(path) && !fs::create_directories(path))
      throw std::exception("Could not create the CtrlShiftE directory");
    return path;
  }();

  return path;
}

fs::path getUserConfigFilePath(const char *fileName, const char *defaultFileContents)
{
  fs::path path = getUserFilesPath() / fileName;
  if (!fs::is_regular_file(path)) {
    std::ofstream file{ path };
    file << defaultFileContents << std::endl;
  }
  return path;
}

void runLater(std::function<void()> &&executor)
{
  s_delayedTasks.push_back(executor);
}

void runDetached(std::function<void()> &&call)
{
  std::thread _launcher([call=std::move(call)] {
    call();
  });
  _launcher.detach();
}

void runDelayedTasks()
{
  for (size_t i = 0; i < s_delayedTasks.size(); i++)
    s_delayedTasks[i]();
  s_delayedTasks.clear();
}

void openWebPage(const char *url)
{
  cse::log("Opening web page" + std::string(url));
  ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
}

void openFileDir(const char *path)
{
  cse::log("Opening directory " + std::string(path));
  ShellExecuteA(NULL, "explore", path, NULL, NULL, SW_SHOWNORMAL);
}

void executeShellCommand(const std::string &command, bool interactive/*=false*/)
{
  if (command.empty()) {
    log("Tried to execute an empty command");
    return;
  }
  std::string file;
  std::string params;
  if(command[0] == '"') {
    size_t split = command.find('"');
    if(split == std::string::npos) {
      logm("Invalid shell command: ", command);
      return;
    }
    file = command.substr(1, split);
    split++;
    while (split < command.size() && command[split] == ' ')
      split++;
    params = command.substr(split);
  } else {
    size_t split = command.find(' ');
    file = command.substr(0, split);
    if (split != std::string::npos)
      params = command.substr(split+1);
  }

  auto success = ShellExecuteA(NULL, "open", file.c_str(), params.c_str(), NULL, interactive ? SW_SHOWNORMAL : SW_HIDE);
  logm("Executed shell command '", file, "' with params '", params, "'",
       interactive ? " (interactive)" : "", ", got ", std::to_string(reinterpret_cast<long long>(success)));
}

}
