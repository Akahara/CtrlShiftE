#pragma once

#include <functional>
#include <memory>

typedef unsigned char KeyFlags;

enum KeyFlags_ : KeyFlags {
  KeyFlags_None   = 0,
  KeyFlags_Option = 1 << 0,
  KeyFlags_Ctrl   = 1 << 1,
  KeyFlags_Shift  = 1 << 2,
};

enum PressType {
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

  bool isOptionPressed() const { return keyFlags & KeyFlags_Option; }
  bool isCtrlPressed()   const { return keyFlags & KeyFlags_Ctrl; }
  bool isShiftPressed()  const { return keyFlags & KeyFlags_Shift; }
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

  bool doesStrokeMatch(const GlobalKeyEvent &ks) const;
};

class GlobalKeyListener {
public:
  GlobalKeyListener() = default;
  virtual ~GlobalKeyListener() = default;

  GlobalKeyListener(const GlobalKeyListener &) = delete;
  GlobalKeyListener &operator=(const GlobalKeyListener &) = delete;
  GlobalKeyListener(GlobalKeyListener &&) = delete;
  GlobalKeyListener &operator=(GlobalKeyListener &&) = delete;

  virtual void onKeyPressed(const GlobalKeyEvent &ev) = 0;
  virtual void onButtonPressed(const GlobalButtonEvent &ev) = 0;
};

struct CursorPosition {
  long cursorX, cursorY;
  long virtualViewportWidth, virtualViewportHeight;
};

namespace cse::keys
{

void addGlobalySuppressedKeystroke(GlobalKeystroke keystroke);
void addGlobalKeyListener(const std::shared_ptr<GlobalKeyListener> &listener);
void captureNextClick(std::function<void(const GlobalButtonEvent &)> &&callback);
void sendKeyImmediate(unsigned short scanCode, unsigned short vkCode);
void sendKey(const GlobalKeyEvent &key);
void sendButton(const GlobalButtonEvent &btn);
void prepareEventsDispatch(); // should be called after physical monitors change

CursorPosition getScreenCursor();

#ifdef CSE_EXPOSE_INTERNALS

void loadResources();
void registerGlobalHook();
void disposeHookAndResources();

// called by the main thread to process global events
void pollEvents();

#endif

}