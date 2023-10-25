#define CSE_EXPOSE_INTERNALS
#include "cse_extensions.h"
#undef CSE_EXPOSE_INTERNALS

#include <Windows.h>

namespace cse::extensions {

static const fs::path USER_FILES_DIR = ".CtrlShiftE";

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

Executor runLater(Executor executor)
{
  return [executor](const auto &args) {
    std::thread _launcher([executor, &args] {
      executor(args);
    });
    _launcher.detach();
  };
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

}
