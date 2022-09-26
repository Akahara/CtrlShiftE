#include "cse_internal.h"

#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

#include <Windows.h>

#include "graphics.h"

static struct KeyEventListener {
  std::thread listenerThread;
  std::vector<GlobalKeyEvent> polledEvents;
  std::mutex polledEventsMutex;
} *globalKeyListener;

static std::vector<GlobalKeystroke> suppressedKeystrokes;
static std::vector<std::shared_ptr<GlobalKeyListener>> globalKeyListeners;

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
  long long eventTime = currentTimeMillis(); // cannot use keyEvent->time because it is relative to when the computed booted
  GlobalKeyEvent ev{ keyEvent->vkCode, flags, eventTime };

  {
    std::lock_guard _lock{ globalKeyListener->polledEventsMutex };
    globalKeyListener->polledEvents.push_back(ev);
  }

  // return true iff the event must be suppressed

  for (GlobalKeystroke &ks : suppressedKeystrokes) {
    if (ks.match(ev))
      return true;
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

static void globalKeyListenerEntryPoint()
{
  cse::log("Listening for global keypresses");
  // WH_KEYBOARD cannot be caught because the event is only propagated to the active window
  // otherwise there would be no need for another thread
  SetWindowsHookExA(WH_KEYBOARD_LL, globalKeyboardHookProc, NULL, NULL);
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
  globalKeyListener->polledEventsMutex.lock();
  // take ownership of the events list to release the mutex asap
  // the list is also cleared for immediate reuse
  std::vector<GlobalKeyEvent> events = std::move(globalKeyListener->polledEvents);
  globalKeyListener->polledEventsMutex.unlock();

  for (GlobalKeyEvent &ev : events) {
    for (auto &listener : globalKeyListeners)
      listener->onKeyPressed(ev);
  }

}

}