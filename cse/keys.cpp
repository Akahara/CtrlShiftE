#include "cse_internal.h"

#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

#include <Windows.h>

#include <bitset>
#include "graphics.h"

static struct KeyEventListener {
  std::thread listenerThread;
  std::vector<GlobalKeyEvent> polledKeyEvents;
  std::vector<GlobalButtonEvent> polledButtonEvents;
  std::function<void(const GlobalButtonEvent&)> nextClickCaptureCallback;
  std::vector<DWORD> pressedKeys;
  bool wantsNextClickCapture = false;
  std::mutex polledEventsMutex;
} *globalKeyListener;

static std::vector<GlobalKeystroke> suppressedKeystrokes;
static std::vector<std::shared_ptr<GlobalKeyListener>> globalKeyListeners;

static struct MousePosition {
  long cursorX, cursorY;
  int virtualViewportWidth, virtualViewportHeight;
} currentCursor;

inline long long currentTimeMillis()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

static bool processKey(KBDLLHOOKSTRUCT *keyEvent)
{
  KeyFlags flags = 0;
  flags |= (keyEvent->flags & LLKHF_ALTDOWN) ? KeyFlags_Option  : 0;
  flags |= GetAsyncKeyState(VK_CONTROL)      ? KeyFlags_Ctrl    : 0;
  flags |= GetAsyncKeyState(VK_SHIFT)        ? KeyFlags_Shift   : 0;
  long long eventTime = currentTimeMillis(); // cannot use keyEvent->time because it is relative to when the computer booted
  GlobalKeyEvent ev{};
  bool pressed = !(keyEvent->flags & LLKHF_UP);
  auto previousPressed = std::find(globalKeyListener->pressedKeys.begin(), globalKeyListener->pressedKeys.end(), keyEvent->vkCode);
  bool wasPreviouslyPressed = previousPressed != globalKeyListener->pressedKeys.end();
  ev.keyPress = !pressed ? PressType_Release : (wasPreviouslyPressed ? PressType_Repeat : PressType_Press);
  ev.keyCode = (unsigned char)keyEvent->vkCode;
  ev.scanCode = (unsigned char)keyEvent->scanCode;
  ev.keyFlags = flags;
  ev.pressTime = eventTime;

  if (pressed && !wasPreviouslyPressed)
    globalKeyListener->pressedKeys.emplace_back(keyEvent->vkCode);
  else if (!pressed && wasPreviouslyPressed)
    globalKeyListener->pressedKeys.erase(previousPressed);

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
  ev.button    = (int)((eventWParam - WM_LBUTTONDOWN) / 2);
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
  return processKey((KBDLLHOOKSTRUCT *)lParam);
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

void sendKeyImmediate(unsigned short scanCode, unsigned short vkCode)
{
  INPUT inputs[2]{};
  inputs[0].type = INPUT_KEYBOARD;
  inputs[0].ki.wScan = scanCode;
  inputs[0].ki.wVk = vkCode;
  inputs[1].type = INPUT_KEYBOARD;
  inputs[1].ki.wScan = scanCode;
  inputs[1].ki.wVk = vkCode;
  inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(2, inputs, sizeof INPUT);
}

void sendKey(const GlobalKeyEvent &key)
{
  INPUT input{};
  input.type = INPUT_KEYBOARD;
  input.ki.wScan = key.scanCode;
  input.ki.wVk = key.keyCode;
  input.ki.dwFlags |= key.keyPress == PressType_Release ? KEYEVENTF_KEYUP : 0;
  SendInput(1, &input, sizeof INPUT);
}

void sendButton(const GlobalButtonEvent &btn)
{
  INPUT input{};
  input.type = INPUT_MOUSE;
  input.mi.dx = btn.cursorX * 65535 / currentCursor.virtualViewportWidth;
  input.mi.dy = btn.cursorY * 65535 / currentCursor.virtualViewportHeight;
  input.mi.dwFlags |= MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
  input.mi.dwFlags |= btn.isPressed ? (btn.button == 0 ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN) : (btn.button == 0 ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP);
  SendInput(1, &input, sizeof INPUT);
}

void prepareEventsDispatch()
{
  SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  currentCursor.virtualViewportWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  currentCursor.virtualViewportHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
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