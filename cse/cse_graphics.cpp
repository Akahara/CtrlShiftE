#define CSE_EXPOSE_INTERNALS
#include "cse_graphics.h"
#undef CSE_EXPOSE_INTERNALS

#include <iostream>
#include <vector>
#include <d3d9.h>
#include <tchar.h>
#include <dwmapi.h>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

#include "cse_extensions.h"
#include "cse_utils.h"

struct {
  HWND                  wnd;
  LPDIRECT3D9           pd3d = NULL;
  LPDIRECT3DDEVICE9     pd3dDevice = NULL;
  D3DPRESENT_PARAMETERS d3dPP = {};
} g_mainWindow;

static std::vector<cse::graphics::Window> activeWindows;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
  case WM_SIZE:
    if (g_mainWindow.pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
      g_mainWindow.d3dPP.BackBufferWidth = LOWORD(lParam);
      g_mainWindow.d3dPP.BackBufferHeight = HIWORD(lParam);
      void resetDeviceD3D();
      resetDeviceD3D();
    }
    return 0;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
      return 0;
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  default: ;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

static bool createDeviceD3D(HWND hWnd)
{
  if ((g_mainWindow.pd3d = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
    return false;

  ZeroMemory(&g_mainWindow.d3dPP, sizeof(g_mainWindow.d3dPP));
  g_mainWindow.d3dPP.Windowed = TRUE;
  g_mainWindow.d3dPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
  g_mainWindow.d3dPP.BackBufferFormat = D3DFMT_UNKNOWN;
  g_mainWindow.d3dPP.EnableAutoDepthStencil = TRUE;
  g_mainWindow.d3dPP.AutoDepthStencilFormat = D3DFMT_D16;
  g_mainWindow.d3dPP.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
  if (g_mainWindow.pd3d->CreateDevice(
    D3DADAPTER_DEFAULT,
    D3DDEVTYPE_HAL,
    hWnd,
    D3DCREATE_HARDWARE_VERTEXPROCESSING,
    &g_mainWindow.d3dPP,
    &g_mainWindow.pd3dDevice) < 0)
    return false;

  return true;
}

void resetDeviceD3D()
{
  ImGui_ImplDX9_InvalidateDeviceObjects();
  if (g_mainWindow.pd3dDevice->Reset(&g_mainWindow.d3dPP) == D3DERR_INVALIDCALL)
    IM_ASSERT(0);
  ImGui_ImplDX9_CreateDeviceObjects();
}

static void cleanupDeviceD3D()
{
  if (g_mainWindow.pd3dDevice) {
    g_mainWindow.pd3dDevice->Release();
    g_mainWindow.pd3dDevice = NULL;
  }
  if (g_mainWindow.pd3d) {
    g_mainWindow.pd3d->Release();
    g_mainWindow.pd3d = NULL;
  }
}



WindowProcess::WindowProcess(std::string_view windowName)
  : m_windowName(windowName)
{
}

bool WindowProcess::beginWindow()
{
  return ImGui::Begin(m_windowName.c_str(), &m_isVisible, ImGuiViewportFlags_NoTaskBarIcon);
}


namespace cse::graphics {

void loadGraphics()
{
  // create a hidden main window
  // (necessary for ImGui, but never actually used)
  WNDCLASSEX cw;
  cw.cbSize = sizeof(WNDCLASSEX);
  cw.style = CS_CLASSDC;
  cw.lpfnWndProc = WndProc;
  cw.cbClsExtra = 0L;
  cw.cbWndExtra = 0L;
  cw.hInstance = GetModuleHandle(NULL);
  cw.hIcon = NULL;
  cw.hCursor = NULL;
  cw.hbrBackground = NULL;
  cw.lpszMenuName = NULL;
  cw.lpszClassName = "CtrlShiftE_Class";
  cw.hIconSm = NULL;
  RegisterClassEx(&cw);

  g_mainWindow.wnd = CreateWindow(
    cw.lpszClassName,
    "CtrlShiftE",
    WS_OVERLAPPEDWINDOW,
    0, 0, 1, 100,
    NULL,
    NULL,
    cw.hInstance,
    NULL
  );

  if (!createDeviceD3D(g_mainWindow.wnd))
    throw std::runtime_error("Could not initialize Direct3D!");

  UpdateWindow(g_mainWindow.wnd);

  // Create ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  std::string iniFilePath = (cse::extensions::getUserFilesPath() / "imgui.ini").string();
  io.IniFilename = _strdup(iniFilePath.c_str()); // not freed but must be kept until imgui is destroyed (the whole program's lifetime)
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigViewportsNoTaskBarIcon = true;

  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowPadding.x = 0;
  style.WindowPadding.y = 0;
  style.WindowRounding = 0.0f;

  ImGui_ImplWin32_Init(g_mainWindow.wnd);
  ImGui_ImplDX9_Init(g_mainWindow.pd3dDevice);
}

static void removeClosedWindows()
{
  for (size_t i = 0; i < activeWindows.size(); ) {
    if (activeWindows[i].process->isVisible())
      i++;
    else
      activeWindows.erase(activeWindows.begin() + i);
  }
}

void destroyGraphics()
{
  ImGui_ImplDX9_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  cleanupDeviceD3D();

  DestroyWindow(g_mainWindow.wnd);
}

void render()
{
  // process inputs
  MSG msg;
  while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // begin frame
  ImGui_ImplDX9_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  // draw frame
  for (Window &window : activeWindows) {
    if (window.process->isVisible()) {
      prepareAlwaysOnTop();
      bool visible = window.process->beginWindow();
      if (visible) window.process->render();
      ImGui::End();
    }
  }

  // Send frame
  ImGui::EndFrame();

  if (g_mainWindow.pd3dDevice->BeginScene() >= 0) {
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    g_mainWindow.pd3dDevice->EndScene();
  }

  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();

  if (g_mainWindow.pd3dDevice->Present(NULL, NULL, NULL, NULL) == D3DERR_DEVICELOST
      && g_mainWindow.pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
    resetDeviceD3D();

  removeClosedWindows();
}

bool shouldDispose()
{
  return activeWindows.empty();
}

void createWindow(const std::shared_ptr<WindowProcess> &process)
{
  if (std::ranges::any_of(activeWindows, [&](const Window &win) { return win.process->getName() == process->getName(); }))
    return;
  Window win{ process };
  activeWindows.push_back(std::move(win));
  cse::log("Created window " + process->getName());
}

void closeAllWindows()
{
  std::ranges::for_each(activeWindows, [](Window &win) { win.process->setVisible(false); }); // FIX is that necessary ?
  activeWindows.clear();
}

void prepareAlwaysOnTop()
{
  ImGuiWindowClass activeClass;
  activeClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
  ImGui::SetNextWindowClass(&activeClass);
}

}