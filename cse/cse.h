#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <filesystem>
#include <thread>

#include "commands.h"

typedef unsigned char KeyFlags;

enum KeyFlags_ : KeyFlags {
  KeyFlags_None   = 0,
  KeyFlags_Option = 1 << 0,
  KeyFlags_Ctrl   = 1 << 1,
  KeyFlags_Shift  = 1 << 2,
};

struct GlobalKeyEvent {
  char keyCode;
  KeyFlags keyFlags;
  long long pressTime;
};

struct GlobalButtonEvent {
  int button;
  bool isPressed;
  long long pressTime;
};

struct GlobalKeystroke {
  char keyCode;
  KeyFlags keyFlags;
public:
  bool match(const GlobalKeyEvent &ks) const;
};

class GlobalKeyListener {
public:
  virtual ~GlobalKeyListener() = default;

  virtual void onKeyPressed(GlobalKeyEvent ev) = 0;
  virtual void onButtonPressed(GlobalButtonEvent ev) = 0;
};

namespace cse {

void log(std::string_view line);
void logErr(std::string_view line);
void logInfo(std::string_view line);

void addCommand(Command &&command);

const std::filesystem::path &getUserFilesPath();

namespace keys {

void addGlobalySuppressedKeystroke(GlobalKeystroke keystroke);
void addGlobalKeyListener(const std::shared_ptr<GlobalKeyListener> &listener);

}

namespace extensions {

/*
 * CSE extensions are instanciated after core functions
 * and are destroyed before theim. It is safe to put
 * initialization and cleanup code in constructors/destructors
 * (RAII)
 */
class CSEExtension {
public:
  virtual ~CSEExtension() {}
};

static Executor runLater(Executor executor)
{
  return [executor](const auto &args) {
    // run in separate thread to avoid stalling the main thread
    // until the command starts
    std::thread _launcher([executor, &args]() {
      executor(args);
    });
    _launcher.detach();
  };
}

}

}
