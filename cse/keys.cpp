#include "cse_internal.h"

#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

#include <Windows.h>

#include "graphics.h"

static struct KeyEventListener {
  std::thread listenerThread;
  std::vector<GlobalKeyEvent> polledKeyEvents;
  std::vector<GlobalButtonEvent> polledButtonEvents;
  std::function<void(const GlobalButtonEvent&)> nextClickCaptureCallback;
  bool wantsNextClickCapture = false;
  std::mutex polledEventsMutex;
} *globalKeyListener;

static std::vector<GlobalKeystroke> suppressedKeystrokes;
static std::vector<std::shared_ptr<GlobalKeyListener>> globalKeyListeners;

static struct MousePosition {
  long cursorX, cursorY;
} currentCursor;

inline long long currentTimeMillis()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

static bool processKey(KBDLLHOOKSTRUCT *keyEvent)
{
  KeyFlags flags = 0;
  flags |= (keyEvent->flags & LLKHF_ALTDOWN) ? KeyFlags_Option : 0;
  flags |= GetAsyncKeyState(VK_CONTROL)      ? KeyFlags_Ctrl   : 0;
  flags |= GetAsyncKeyState(VK_SHIFT)        ? KeyFlags_Shift  : 0;
  long long eventTime = currentTimeMillis(); // cannot use keyEvent->time because it is relative to when the computer booted
  GlobalKeyEvent ev{};
  ev.keyCode = (unsigned char)keyEvent->vkCode;
  ev.scanCode = (unsigned char)keyEvent->scanCode;
  ev.keyFlags = flags;
  ev.pressTime = eventTime;

  {
    std::lock_guard _lock{ globalKeyListener->polledEventsMutex };
    globalKeyListener->polledKeyEvents.push_back(ev);
  }

  // return true iff the event must be suppressed
  for (GlobalKeystroke &ks : suppressedKeystrokes) {
    if (ks.match(ev))
      return true;
  }
  return false;
}

static bool processMouse(MSLLHOOKSTRUCT *mouseEvent, WPARAM eventWParam)
{
  long long eventTime = currentTimeMillis(); // cannot use keyEvent->time because it is relative to when the computer booted
  GlobalButtonEvent ev{};
  // optimization: WM_[L/R]BUTTON[DOWN/UP] are 0x201..0x205
  ev.button    = (eventWParam - WM_LBUTTONDOWN) / 2;
  ev.isPressed = eventWParam & 1;
  ev.pressTime = eventTime;
  ev.cursorX = mouseEvent->pt.x;
  ev.cursorY = mouseEvent->pt.y;

  if (globalKeyListener->wantsNextClickCapture) {
    std::lock_guard _lock{ globalKeyListener->polledEventsMutex };
    globalKeyListener->nextClickCaptureCallback(ev);
    globalKeyListener->nextClickCaptureCallback = [](const auto&) {}; // free closure arguments, may be slow?
    globalKeyListener->wantsNextClickCapture = false;
    return true;
  }

  {
    std::lock_guard _lock{ globalKeyListener->polledEventsMutex };
    globalKeyListener->polledButtonEvents.push_back(ev);
  }

  return false;
}

static LRESULT CALLBACK globalKeyboardHookProc(int code, WPARAM wParam, LPARAM lParam)
{
  if(code < 0)
    return CallNextHookEx(0, code, wParam, lParam);
  if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
    return processKey((KBDLLHOOKSTRUCT *)lParam);
  return CallNextHookEx(0, code, wParam, lParam);
}

static LRESULT CALLBACK globalMouseHookProc(int code, WPARAM wParam, LPARAM lParam)
{
  if (code < 0)
    return CallNextHookEx(0, code, wParam, lParam);
  MSLLHOOKSTRUCT *mouseData = (MSLLHOOKSTRUCT *)lParam;
  if (wParam == WM_MOUSEMOVE) {
    currentCursor.cursorX = mouseData->pt.x;
    currentCursor.cursorY = mouseData->pt.y;
  }
  if (wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP || wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP) {
    if (processMouse(mouseData, wParam))
      return 1;
  }
  return CallNextHookEx(0, code, wParam, lParam);
}

static void globalKeyListenerEntryPoint()
{
  cse::log("Listening for global keypresses");
  // WH_KEYBOARD cannot be caught because the event is only propagated to the active window
  // otherwise there would be no need for another thread
  SetWindowsHookExA(WH_KEYBOARD_LL, globalKeyboardHookProc, NULL, NULL);
  SetWindowsHookExA(WH_MOUSE_LL, globalMouseHookProc, NULL, NULL);
  int errorFlag;
  MSG msg;
  while ((errorFlag = GetMessage(&msg, NULL, 0, 0)) != 0) {
    if (errorFlag == -1) {
      cse::logErr("Received an invalid win32 message");
    } else {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

namespace cse::keys {


void addGlobalySuppressedKeystroke(GlobalKeystroke keystroke)
{
  suppressedKeystrokes.push_back(keystroke);
}

void addGlobalKeyListener(const std::shared_ptr<GlobalKeyListener> &listener)
{
  globalKeyListeners.push_back(listener);
}

void captureNextClick(std::function<void(const GlobalButtonEvent&)> &&callback)
{
  std::lock_guard _lock{ globalKeyListener->polledEventsMutex };
  globalKeyListener->nextClickCaptureCallback = std::move(callback);
  globalKeyListener->wantsNextClickCapture = true;
}

long getScreenCursorX()
{
  return currentCursor.cursorX;
}

long getScreenCursorY()
{
  return currentCursor.cursorY;
}

void registerGlobalHook()
{
  globalKeyListener = new KeyEventListener{
    std::thread{ globalKeyListenerEntryPoint }
  };
}

void unregisterGlobalHook()
{
  globalKeyListeners.clear();

  // TODO find out how to properly terminate the global keys listener thread
  // currently the thread stops when the main function returns, whether it
  // was detached or not, that does not seem to be well-defined by the standard.
  // (but it is what we're aiming for)
  
  //std::terminate();
  //delete globalKeysListenerThread;
}

void pollEvents()
{
  if (!globalKeyListener->polledEventsMutex.try_lock()) {
    cse::log(">nolock");
    return; // if the mutex was busy return and try again on the next app frame (1/60th of a second does not matter much)
  }
  // take ownership of the events list to release the mutex asap
  // the list is also cleared for immediate reuse
  std::vector<GlobalKeyEvent> keyEvents = std::move(globalKeyListener->polledKeyEvents);
  std::vector<GlobalButtonEvent> buttonEvents = std::move(globalKeyListener->polledButtonEvents);
  globalKeyListener->polledEventsMutex.unlock();

  for (GlobalKeyEvent &ev : keyEvents) {
    for (auto &listener : globalKeyListeners)
      listener->onKeyPressed(ev);
  }
  for (GlobalButtonEvent &ev : buttonEvents) {
    for (auto &listener : globalKeyListeners)
      listener->onButtonPressed(ev);
  }
}

}