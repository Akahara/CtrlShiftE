#define CSE_EXPOSE_INTERNALS
#include "cse_keys.h"
#undef CSE_EXPOSE_INTERNALS

#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

#include <Windows.h>

#include "cse_utils.h"

static struct KeyEventListener {
  std::thread listenerThread;
  std::vector<GlobalKeyEvent> polledKeyEvents;
  std::vector<GlobalButtonEvent> polledButtonEvents;
  std::function<void(const GlobalButtonEvent&)> nextClickCaptureCallback;
  std::vector<DWORD> pressedKeys;
  bool wantsNextClickCapture = false;
  std::mutex polledEventsMutex;
} *g_globalKeyListener;

static struct GlobalData
{
  std::vector<GlobalKeystroke>                    suppressedKeystrokes;
  std::vector<std::shared_ptr<GlobalKeyListener>> globalKeyListeners;
  CursorPosition                                  screenCursor;
} *g_globalData;

bool GlobalKeystroke::doesStrokeMatch(const GlobalKeyEvent &ev) const
{
  return keyCode == ev.keyCode && (keyFlags & ev.keyFlags) == keyFlags;
}

static long long currentTimeMillis()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

static bool processKey(const KBDLLHOOKSTRUCT *keyEvent)
{
  KeyFlags flags = 0;
  flags |= (keyEvent->flags & LLKHF_ALTDOWN) ? KeyFlags_Option  : 0;
  flags |= GetAsyncKeyState(VK_CONTROL)      ? KeyFlags_Ctrl    : 0;
  flags |= GetAsyncKeyState(VK_SHIFT)        ? KeyFlags_Shift   : 0;
  long long eventTime = currentTimeMillis(); // cannot use keyEvent->time because it is relative to when the computer booted
  GlobalKeyEvent ev{};
  bool pressed = !(keyEvent->flags & LLKHF_UP);
  auto previousPressed = std::ranges::find(g_globalKeyListener->pressedKeys, keyEvent->vkCode);
  bool wasPreviouslyPressed = previousPressed != g_globalKeyListener->pressedKeys.end();
  ev.keyPress = !pressed ? PressType_Release : (wasPreviouslyPressed ? PressType_Repeat : PressType_Press);
  ev.keyCode = static_cast<unsigned char>(keyEvent->vkCode);
  ev.scanCode = static_cast<unsigned char>(keyEvent->scanCode);
  ev.keyFlags = flags;
  ev.pressTime = eventTime;

  if (pressed && !wasPreviouslyPressed)
    g_globalKeyListener->pressedKeys.emplace_back(keyEvent->vkCode);
  else if (!pressed && wasPreviouslyPressed)
    g_globalKeyListener->pressedKeys.erase(previousPressed);

  {
    std::lock_guard _lock{ g_globalKeyListener->polledEventsMutex };
    g_globalKeyListener->polledKeyEvents.push_back(ev);
  }

  // return true iff the event must be suppressed
  return std::ranges::any_of(g_globalData->suppressedKeystrokes,
    [&ev](const auto &stroke) { return stroke.doesStrokeMatch(ev); });
}

static bool processMouse(const MSLLHOOKSTRUCT *mouseEvent, WPARAM eventWParam)
{
  long long eventTime = currentTimeMillis(); // cannot use keyEvent->time because it is relative to when the computer booted
  GlobalButtonEvent ev{};
  // optimization: WM_[L/R]BUTTON[DOWN/UP] are 0x201..0x205
  ev.button    = static_cast<int>((eventWParam - WM_LBUTTONDOWN) / 2);
  ev.isPressed = eventWParam & 1;
  ev.pressTime = eventTime;
  ev.cursorX = mouseEvent->pt.x;
  ev.cursorY = mouseEvent->pt.y;

  if (g_globalKeyListener->wantsNextClickCapture) {
    std::lock_guard _lock{ g_globalKeyListener->polledEventsMutex };
    g_globalKeyListener->nextClickCaptureCallback(ev);
    g_globalKeyListener->nextClickCaptureCallback = {}; // free closure arguments
    g_globalKeyListener->wantsNextClickCapture = false;
    return true;
  }

  {
    std::lock_guard _lock{ g_globalKeyListener->polledEventsMutex };
    g_globalKeyListener->polledButtonEvents.push_back(ev);
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
    g_globalData->screenCursor.cursorX = mouseData->pt.x;
    g_globalData->screenCursor.cursorY = mouseData->pt.y;
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
  g_globalData->suppressedKeystrokes.push_back(keystroke);
}

void addGlobalKeyListener(const std::shared_ptr<GlobalKeyListener> &listener)
{
  g_globalData->globalKeyListeners.push_back(listener);
}

void captureNextClick(std::function<void(const GlobalButtonEvent&)> &&callback)
{
  std::lock_guard _lock{ g_globalKeyListener->polledEventsMutex };
  g_globalKeyListener->nextClickCaptureCallback = std::move(callback);
  g_globalKeyListener->wantsNextClickCapture = true;
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
  input.mi.dx = btn.cursorX * 65535 / g_globalData->screenCursor.virtualViewportWidth;
  input.mi.dy = btn.cursorY * 65535 / g_globalData->screenCursor.virtualViewportHeight;
  input.mi.dwFlags |= MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
  input.mi.dwFlags |= btn.isPressed ? (btn.button == 0 ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN) : (btn.button == 0 ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP);
  SendInput(1, &input, sizeof INPUT);
}

void prepareEventsDispatch()
{
  SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  g_globalData->screenCursor.virtualViewportWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  g_globalData->screenCursor.virtualViewportHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
}

CursorPosition getScreenCursor()
{
  return g_globalData->screenCursor;
}

void loadResources()
{
  g_globalData = new GlobalData;
}

void registerGlobalHook()
{
  g_globalKeyListener = new KeyEventListener{
    std::thread{ globalKeyListenerEntryPoint }
  };
}

void disposeHookAndResources()
{
  delete g_globalData;

  // TODO find out how to properly terminate the global keys listener thread
  // currently the thread stops when the main function returns, whether it
  // was detached or not, that does not seem to be well-defined by the standard.
  // (but it is what we're aiming for)
}

void pollEvents()
{
  if (!g_globalKeyListener->polledEventsMutex.try_lock()) {
    return; // if the mutex was busy return and try again on the next app frame (1 frame latency does not matter much)
  }
  // take ownership of the events list to release the mutex asap
  // the list is also cleared for immediate reuse
  std::vector<GlobalKeyEvent> keyEvents = std::move(g_globalKeyListener->polledKeyEvents);
  std::vector<GlobalButtonEvent> buttonEvents = std::move(g_globalKeyListener->polledButtonEvents);
  g_globalKeyListener->polledEventsMutex.unlock();

  for (GlobalKeyEvent &ev : keyEvents) {
    for (auto &listener : g_globalData->globalKeyListeners)
      listener->onKeyPressed(ev);
  }
  for (GlobalButtonEvent &ev : buttonEvents) {
    for (auto &listener : g_globalData->globalKeyListeners)
      listener->onButtonPressed(ev);
  }
}

}