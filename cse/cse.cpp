#include "cse.h"
#include "cse_internal.h"

#include <filesystem>

#include <Windows.h>

#include "extensions/debug_window.h"

namespace fs = std::filesystem;

static const fs::path USER_FILES_DIR = ".CtrlShiftE";
static std::vector<Command> iotGenerators;


void cse::log(std::string_view line)
{
  DebugWindowProcess::log(line);
}

void cse::logErr(std::string_view line)
{
  cse::log(line);
}

void cse::logInfo(std::string_view line)
{
  cse::log(line);
}

void cse::addCommand(Command &&command)
{
  log(std::string("Added command ") + command.prefix);
  iotGenerators.push_back(std::move(command));
}

static fs::path getOrCreateUserFilesPath()
{
  char *home;
  size_t homeLength;
  _dupenv_s(&home, &homeLength, "USERPROFILE");
  if (home == nullptr)
    throw std::exception("Could not retrieve user's home");
  fs::path path = home / USER_FILES_DIR;
  if (!fs::is_directory(path) && !fs::create_directories(path))
    throw std::exception("Could not create the CtrlShiftE directory");
  return path;
}

const fs::path &cse::getUserFilesPath()
{
  static fs::path path = getOrCreateUserFilesPath();
  return path;
}

std::vector<Command> &cse::getCommands()
{
  return iotGenerators;
}

WindowProcess::WindowProcess(std::string_view windowName)
  : m_windowName(windowName)
{
}

bool WindowProcess::beginWindow()
{
  return ImGui::Begin(m_windowName.c_str(), &m_isVisible, ImGuiViewportFlags_NoTaskBarIcon);
}

bool GlobalKeystroke::match(const GlobalKeyEvent &ev) const
{
  return keyCode == ev.keyCode && (keyFlags & ev.keyFlags) == keyFlags;
}

namespace cse::extensions {

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