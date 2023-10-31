#define CSE_EXPOSE_INTERNALS
#include "cse_extensions.h"
#undef CSE_EXPOSE_INTERNALS

#include <fstream>
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

fs::path getUserConfigFilePath(const char *fileName, const char *defaultFileContents)
{
  fs::path path = getUserFilesPath() / fileName;
  if (!fs::is_regular_file(path)) {
    std::ofstream file{ path };
    file << defaultFileContents << std::endl;
  }
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

void runDetached(std::function<void()> &&call)
{
  std::thread _launcher([call=std::move(call)] {
    call();
  });
  _launcher.detach();
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

void executeShellCommand(const char* cmd)
{
  cse::logm("Unimplemented executeShellCommand(", cmd, ")");
  //SHELLEXECUTEINFOA info;
  //ZeroMemory(&info, sizeof(info));
  //info.cbSize = sizeof(SHELLEXECUTEINFOA);
  //info.fMask = SEE_MASK_NO_CONSOLE;
  //ShellExecuteExA(&info);
}

}
