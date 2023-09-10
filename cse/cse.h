#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <filesystem>
#include <thread>

#include "commands.h"

#define CSE_LOGS(logs, stream) { std::stringstream ss; ss << logs; cse::log##stream(ss.str()); }
#define CSE_LOG(logs) CSE_LOGS(logs, )
#define CSE_LOGINF(logs) CSE_LOGS(logs, Info)
#define CSE_LOGERR(logs) CSE_LOGS(logs, Err)

typedef unsigned char KeyFlags;

enum KeyFlags_ : KeyFlags {
  KeyFlags_None    = 0,
  KeyFlags_Option  = 1 << 0,
  KeyFlags_Ctrl    = 1 << 1,
  KeyFlags_Shift   = 1 << 2,
};

enum PressType
{
  PressType_Release,
  PressType_Press,
  PressType_Repeat,
};

struct GlobalKeyEvent {
  unsigned char keyCode = 0;
  unsigned char scanCode = 0;
  PressType keyPress = PressType_Release;
  KeyFlags keyFlags = KeyFlags_None;
  long long pressTime = 0;

  bool isOptionPressed() const { return keyFlags & KeyFlags_Option;  }
  bool isCtrlPressed()   const { return keyFlags & KeyFlags_Ctrl;    }
  bool isShiftPressed()  const { return keyFlags & KeyFlags_Shift;   }
};

struct GlobalButtonEvent {
  int button;
  bool isPressed;
  long long pressTime;
  long cursorX, cursorY;
};

struct GlobalKeystroke {
  unsigned char keyCode;
  KeyFlags keyFlags;

  bool match(const GlobalKeyEvent &ev) const;
  bool matchExactly(const GlobalKeyEvent &ev) const;
};

class GlobalKeyListener {
public:
  virtual ~GlobalKeyListener() = default;

  virtual void onKeyPressed(const GlobalKeyEvent &ev) = 0;
  virtual void onButtonPressed(const GlobalButtonEvent &ev) = 0;
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
void captureNextClick(std::function<void(const GlobalButtonEvent&)> &&callback);
void sendKeyImmediate(unsigned short scanCode, unsigned short vkCode);
void sendKey(const GlobalKeyEvent &key);
void sendButton(const GlobalButtonEvent &btn);
void prepareEventsDispatch(); // should be called after physical monitors change

long getScreenCursorX();
long getScreenCursorY();

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
           CSEExtension() = default;
  virtual ~CSEExtension() = default;
  virtual void reload() {};
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

/*
 * Utility function that opens a web browser to the given url.
 * Beware! This function is fundamentally unsafe when used with
 * unsanitized user input, it may open files or run executables
 * if the url looks like a path. 
 * url must start with http:// or https://, otherwise behavior
 * is undefined.
 */
void openWebPage(const char *url);
void openFileDir(const char *path);

}

}
