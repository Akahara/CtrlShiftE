#include "graphics.h"

#include <iostream>
#include <vector>

#include <d3d9.h>
#include <tchar.h>
#include <dwmapi.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

#include "cse.h"
#include "graphics_spec.h"

struct Window {
  std::shared_ptr<WindowProcess> process;
  //HWND *mainWindow.wnd = nullptr;
};

struct {
  // win32 api
  HWND                  wnd;
  WNDCLASSEX            cw;
  // direct3D
  LPDIRECT3D9           pd3d = NULL;
  LPDIRECT3DDEVICE9     pd3dDevice = NULL;
  D3DPRESENT_PARAMETERS d3dPP = {};
} mainWindow;

static std::vector<Window> activeWindows;
static bool isImGuiDemoWindowVisible = false;



void ResetDeviceD3D()
{
  ImGui_ImplDX9_InvalidateDeviceObjects();
  HRESULT hr = mainWindow.pd3dDevice->Reset(&mainWindow.d3dPP);
  if (hr == D3DERR_INVALIDCALL)
    IM_ASSERT(0);
  ImGui_ImplDX9_CreateDeviceObjects();
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
  case WM_SIZE:
    if (mainWindow.pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
      mainWindow.d3dPP.BackBufferWidth = LOWORD(lParam);
      mainWindow.d3dPP.BackBufferHeight = HIWORD(lParam);
      ResetDeviceD3D();
    }
    return 0;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
      return 0;
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

static bool CreateDeviceD3D(HWND hWnd)
{
  if ((mainWindow.pd3d = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
    return false;

  // Create the D3DDevice
  ZeroMemory(&mainWindow.d3dPP, sizeof(mainWindow.d3dPP));
  mainWindow.d3dPP.Windowed = TRUE;
  mainWindow.d3dPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
  mainWindow.d3dPP.BackBufferFormat = D3DFMT_UNKNOWN;
  mainWindow.d3dPP.EnableAutoDepthStencil = TRUE;
  mainWindow.d3dPP.AutoDepthStencilFormat = D3DFMT_D16;
  mainWindow.d3dPP.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // Present with vsync
  if (mainWindow.pd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &mainWindow.d3dPP, &mainWindow.pd3dDevice) < 0)
    return false;

  return true;
}

static void CleanupDeviceD3D()
{
  if (mainWindow.pd3dDevice) {
    mainWindow.pd3dDevice->Release();
    mainWindow.pd3dDevice = NULL;
  }
  if (mainWindow.pd3d) {
    mainWindow.pd3d->Release();
    mainWindow.pd3d = NULL;
  }
}

void graphics::loadGraphics()
{
  // create a hidden main window
  // (necessary for ImGui, but never actually used)
  mainWindow.cw.cbSize        = sizeof(WNDCLASSEX);
  mainWindow.cw.style         = CS_CLASSDC;
  mainWindow.cw.lpfnWndProc   = WndProc;
  mainWindow.cw.cbClsExtra    = 0L;
  mainWindow.cw.cbWndExtra    = 0L;
  mainWindow.cw.hInstance     = GetModuleHandle(NULL);
  mainWindow.cw.hIcon         = NULL;
  mainWindow.cw.hCursor       = NULL;
  mainWindow.cw.hbrBackground = NULL;
  mainWindow.cw.lpszMenuName  = NULL;
  mainWindow.cw.lpszClassName = L"CtrlShiftE_Class";
  mainWindow.cw.hIconSm       = NULL;
  RegisterClassEx(&mainWindow.cw);
  mainWindow.wnd = CreateWindow(
    mainWindow.cw.lpszClassName,
    L"CtrlShiftE",
    WS_OVERLAPPEDWINDOW,
    0, 0, 1, 100,
    NULL,
    NULL,
    mainWindow.cw.hInstance,
    NULL
  );

  // Create direct3D
  if (!CreateDeviceD3D(mainWindow.wnd)) {
    std::cerr << "Could not initialize Direct3D!" << std::endl;
    exit(1);
  }

  UpdateWindow(mainWindow.wnd);

  // Create ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  std::string iniFilePath = (cse::getUserFilesPath() / "imgui.ini").string();
  io.IniFilename = _strdup(iniFilePath.c_str()); // not freed but must be kept until imgui is destroyed (the whole program's lifetime)
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigViewportsNoTaskBarIcon = true;
  //ImGui::GetWindowViewport()->Flags |= ImGuiViewportFlags_NoTaskBarIcon;

  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowPadding.x = 0;
  style.WindowPadding.y = 0;
  style.WindowRounding = 0.0f;

  ImGui_ImplWin32_Init(mainWindow.wnd);
  ImGui_ImplDX9_Init(mainWindow.pd3dDevice);
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

void graphics::destroyGraphics()
{
  ImGui_ImplDX9_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  CleanupDeviceD3D();

  DestroyWindow(mainWindow.wnd);
  UnregisterClass(mainWindow.cw.lpszClassName, mainWindow.cw.hInstance);
}

void graphics::render()
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
      window_helper::prepareAlwaysOnTop();
      bool visible = window.process->beginWindow();
      if(visible)
        window.process->render();
      ImGui::End();
    }
  }
  
  if(isImGuiDemoWindowVisible)
    ImGui::ShowDemoWindow(&isImGuiDemoWindowVisible);

  // Send frame
  ImGui::EndFrame();
  
  if (mainWindow.pd3dDevice->BeginScene() >= 0) {
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    mainWindow.pd3dDevice->EndScene();
  }

  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();

  HRESULT result = mainWindow.pd3dDevice->Present(NULL, NULL, NULL, NULL);
  if (result == D3DERR_DEVICELOST && mainWindow.pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
    ResetDeviceD3D();

  removeClosedWindows();
}

bool graphics::shouldDispose()
{
  return activeWindows.empty();
}

void graphics::createWindow(const std::shared_ptr<WindowProcess> &process)
{
  for (Window &window : activeWindows) {
    if (window.process->getName() == process->getName())
      return;
  }
  Window win{ process };
  activeWindows.push_back(std::move(win));
  cse::log("Created window " + process->getName());
}

void graphics::closeAllWindows()
{
  for (Window &win : activeWindows) {
    win.process->setVisible(false);
  }
  activeWindows.clear();
}

namespace graphics::window_helper {

ImplDetails implDetails;

void prepareTransparent(bool *cond)
{
  if (true)
    return; // disabled, produces strange behaviors with transparency...
  if (*cond) {
    implDetails.nextImGuiViewportTransparent = true;
    *cond = false;
  }
}

void prepareAlwaysOnTop()
{
  ImGuiWindowClass activeClass;
  activeClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
  ImGui::SetNextWindowClass(&activeClass);
}

}