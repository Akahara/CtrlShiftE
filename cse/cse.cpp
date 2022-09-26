#include "cse.h"
#include "cse_internal.h"

#include <filesystem>

#include "extensions/debug_window.h"

namespace fs = std::filesystem;

static const std::string USER_FILES_DIR = "\\.CtrlShiftE";
static std::vector<std::shared_ptr<IOTGenerator>> iotGenerators;


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

void cse::addIOTGenerator(const std::shared_ptr<IOTGenerator> &generator)
{
  log(std::string("Added an IOT generator"));
  iotGenerators.push_back(generator);
}

static std::string getOrCreateUserFilesPath()
{
  char *home;
  size_t homeLength;
  _dupenv_s(&home, &homeLength, "USERPROFILE");
  if (home == nullptr)
    throw std::exception("Could not retrieve user's home");
  std::string path = home + USER_FILES_DIR;
  if (!fs::is_directory(path) && !fs::create_directories(path))
    throw std::exception("Could not create the CtrlShiftE directory");
  return path;
}

const std::string &cse::getUserFilesPath()
{
  static std::string path = getOrCreateUserFilesPath();
  return path;
}

const std::vector<std::shared_ptr<IOTGenerator>> &cse::getIOTGenerators()
{
  return iotGenerators;
}

void cse::clearIOTGenerators()
{
  iotGenerators.clear();
}


WindowProcess::WindowProcess(std::string_view windowName)
  : m_windowName(windowName)
{
}

bool WindowProcess::beginWindow()
{
  return ImGui::Begin(m_windowName.c_str(), &m_isVisible);
}

bool GlobalKeystroke::match(const GlobalKeyEvent &ev) const
{
  return keyCode == ev.keyCode && (keyFlags & ev.keyFlags) == keyFlags;
}
