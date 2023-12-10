#include "color_picker.h"

#include "cse_keys.h"
#include "cse_commands.h"

namespace cse::extensions {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(color_t, r, g, b, a)

static_assert(ColorPickerWindow::CAPTURE_WIDTH == 5 && ColorPickerWindow::CAPTURE_HEIGHT == 5);
static constexpr unsigned int INITIAL_CAPTURE[5*5] {
  0xffdb4dff, 0xffff4df8, 0xffff4dcd, 0xffff4da2, 0xffff4d77,
  0xff4d94ff, 0xff4d69ff, 0xff5b4dff, 0xff864dff, 0xffb04dff,
  0xff4dff94, 0xff4dffbf, 0xff4dffea, 0xff4deaff, 0xff4dbfff,
  0xffdbff4d, 0xffb0ff4d, 0xff86ff4d, 0xff5bff4d, 0xff4dff69,
  0xffff4d4d, 0xffff774d, 0xffffa24d, 0xffffcd4d, 0xfffff84d,
};

ColorPicker::ColorPicker()
{
  cse::commands::addCommand({
    "colorpicker",
    "a colorpicker",
    { /* no arguments */ },
    [](const auto &args) {
      graphics::createWindow(std::make_shared<ColorPickerWindow>());
    }
  });
}

ColorPickerWindow::ColorPickerWindow()
  : WindowProcess("Color Picker")
{
  json config = cse::extensions::getUserGlobalConfig("color_picker");
  m_color = config.contains("color") ? config["color"].get<color_t>() : color_t{1,1,1,1};
  std::ranges::copy(INITIAL_CAPTURE, m_bitmapBuffer);

  refreshFormattedColors(false);

  m_desktopWND = GetDesktopWindow();
  m_desktopDC = GetDC(m_desktopWND);
  m_captureDC = CreateCompatibleDC(m_desktopDC);
  m_captureBitmap = CreateCompatibleBitmap(m_desktopDC, CAPTURE_WIDTH, CAPTURE_HEIGHT);
  SelectObject(m_captureDC, m_captureBitmap);
  ZeroMemory(&m_bitmapInfo, sizeof(BITMAPINFO));
  m_bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  m_bitmapInfo.bmiHeader.biPlanes = 1;
  m_bitmapInfo.bmiHeader.biBitCount = 32;
  m_bitmapInfo.bmiHeader.biWidth = CAPTURE_WIDTH;
  m_bitmapInfo.bmiHeader.biHeight = CAPTURE_HEIGHT;
  m_bitmapInfo.bmiHeader.biCompression = BI_RGB;
}

ColorPickerWindow::~ColorPickerWindow()
{
  json s{ m_color };
  cse::extensions::updateUserGlobalConfig("color_picker", {{ "color", m_color }});

  ReleaseDC(m_desktopWND, m_desktopDC);
  DeleteDC(m_captureDC);
  DeleteObject(m_captureBitmap);
}

void ColorPickerWindow::render()
{
  int refreshFormats = 0;
  bool isShiftDown = ImGui::GetIO().KeyShift;
  ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
  refreshFormats += ImGui::ColorPicker4("## colorpicker", &m_color.r, ImGuiColorEditFlags_NoSidePreview);
  refreshFormats += m_isExtendedDisplay != isShiftDown;
  if (refreshFormats == 0)
    refreshFormattedColors(isShiftDown);
  copiableColorText("## copyrgb", m_rgbText);
  copiableColorText("## copyrgbi", m_rgbIntText);
  copiableColorText("## copyhex", m_hexText);

  m_isExtendedDisplay = isShiftDown;

  if (ImGui::Button("Screen picker")) {
    m_isCapturingScreen = true;
    cse::keys::captureNextClick([this](const GlobalButtonEvent &ev) {
      int rgba = m_bitmapBuffer[CAPTURE_WIDTH * CAPTURE_HEIGHT / 2];
      m_color.r = (rgba >> 16 & 0xff) / 255.f;
      m_color.g = (rgba >> 8  & 0xff) / 255.f;
      m_color.b = (rgba >> 0  & 0xff) / 255.f;
      m_color.a = (rgba >> 24 & 0xff) / 255.f;
      m_isCapturingScreen = false;
    });
  }

  if (m_isCapturingScreen) {
    auto cursor = cse::keys::getScreenCursor();
    BitBlt(m_captureDC, 0, 0, CAPTURE_WIDTH, CAPTURE_HEIGHT,
           m_desktopDC, cursor.cursorX, cursor.cursorY, SRCCOPY | CAPTUREBLT);
    GetDIBits(m_captureDC, m_captureBitmap, 0, CAPTURE_HEIGHT, m_bitmapBuffer, &m_bitmapInfo, DIB_RGB_COLORS);
  }

  ImVec2 c = ImGui::GetCursorScreenPos();
  int s = (int)((ImGui::GetWindowWidth() - 10) / CAPTURE_WIDTH);
  for (int y = 0; y < CAPTURE_HEIGHT; y++) {
    for (int x = 0; x < CAPTURE_WIDTH; x++) {
      int rgba = m_bitmapBuffer[x + y * CAPTURE_WIDTH];
      int r = (rgba >> 16) & 0xff;
      int g = (rgba >> 8 ) & 0xff;
      int b = (rgba >> 0 ) & 0xff;
      int a = (rgba >> 24) & 0xff;
      ImVec2 q = { 5 + c.x + x * s, c.y - y * s + (CAPTURE_HEIGHT - 1) * s };
      ImGui::GetForegroundDrawList()->AddRectFilled(q, { q.x + s, q.y + s }, IM_COL32(r, g, b, 255));
    }
  }
  ImVec2 q = { 5 + c.x + CAPTURE_WIDTH / 2 * s, c.y - (CAPTURE_HEIGHT / 2) * s + (CAPTURE_HEIGHT - 1) * s };
  ImGui::GetForegroundDrawList()->AddRect(q, { q.x + s, q.y + s }, IM_COL32(255, 0, 0, 255));
}

void ColorPickerWindow::refreshFormattedColors(bool extended)
{
  int ri = (int)(255 * m_color.r)
    , gi = (int)(255 * m_color.g)
    , bi = (int)(255 * m_color.b)
    , ai = (int)(255 * m_color.a);
  float r = m_color.r, g = m_color.g, b = m_color.b, a = m_color.a;
  if (m_color.a == 1.f) {
    sprintf_s(m_rgbText, "%s%.3f, %.3f, %.3f%s", extended ? "rgb(" : "", r, g, b, extended ? ")" : "");
    sprintf_s(m_rgbIntText, "%s%3d,%4d,%4d%s", extended ? "rgb(" : "", ri, gi, bi, extended ? ")" : "");
    sprintf_s(m_hexText, "%s%02x%02x%02x", extended ? "0x" : "#", ri, gi, bi);
  } else {
    sprintf_s(m_rgbText, "%s%.3f, %.3f, %.3f, %.3f%s", extended ? "rgba(" : "", r, g, b, a, extended ? ")" : "");
    sprintf_s(m_rgbIntText, "%s%3d,%4d,%4d,%4d%s", extended ? "rgba(" : "", ri, gi, bi, ai, extended ? ")" : "");
    sprintf_s(m_hexText, "%s%02x%02x%02x%02x", extended ? "0x" : "#", ri, gi, bi, ai);
  }
}

void ColorPickerWindow::copiableColorText(const char *formatID, const char *text)
{
  if (ImGui::Selectable(formatID))
    ImGui::SetClipboardText(text);
  ImGui::SameLine();
  ImGui::Text(text);
}

}
