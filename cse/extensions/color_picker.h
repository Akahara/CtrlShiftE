#pragma once

#include <Windows.h>

#include "cse_extensions.h"
#include "cse_graphics.h"

namespace cse::extensions {

class ColorPicker : public CSEExtension {
public:
  ColorPicker();
};

class ColorPickerWindow : public WindowProcess {
public:
  ColorPickerWindow();
  ~ColorPickerWindow() override;

  void render() override;

private:
  void refreshFormattedColors(bool extended);

  static void copiableColorText(const char *formatID, const char *text);

  static constexpr int CAPTURE_WIDTH = 5, CAPTURE_HEIGHT = 5;

  float      m_color[4];
  char       m_rgbText[40];
  char       m_rgbIntText[40];
  char       m_hexText[10];
  bool       m_isExtendedDisplay = false;
  HWND       m_desktopWND;
  HDC        m_desktopDC, m_captureDC;
  HBITMAP    m_captureBitmap;
  BITMAPINFO m_bitmapInfo;
  int        m_bitmapBuffer[CAPTURE_WIDTH * CAPTURE_HEIGHT];
  bool       m_isCapturingScreen = false;
};

}
