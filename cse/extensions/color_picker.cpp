#include "color_picker.h"

#include "../graphics.h"

#include <iostream>

class ColorPickerWindow : public WindowProcess {
private:
  float m_color[4];
  char m_rgbText[33];
  char m_hexText[10];
public:
  ColorPickerWindow()
    : WindowProcess("Color Picker") 
  {
    m_color[0] = m_color[1] = m_color[2] = 1.f;
    m_color[3] = 1.f;
    refreshFormattedColors();
  }

  virtual void render()
  {
    if (ImGui::ColorPicker4("## colorpicker", m_color))
      refreshFormattedColors();
    copiableColorText("## copyrgb", m_rgbText);
    copiableColorText("## copyhex", m_hexText);
  }

private:
  void refreshFormattedColors()
  {
    if (m_color[3] == 1.f) {
      sprintf_s(m_rgbText, "rgb(%.3f, %.3f, %.3f)", m_color[0], m_color[1], m_color[2]);
      sprintf_s(m_hexText, "#%02x%02x%02x", (int)(255 * m_color[0]), (int)(255 * m_color[1]), (int)(255 * m_color[2]));
    } else {
      sprintf_s(m_rgbText, "rgba(%.3f, %.3f, %.3f, %.3f)", m_color[0], m_color[1], m_color[2], m_color[3]);
      sprintf_s(m_hexText, "#%02x%02x%02x%02x", (int)(255 * m_color[0]), (int)(255 * m_color[1]), (int)(255 * m_color[2]), (int)(255 * m_color[3]));
    }
  }

  void copiableColorText(const char *formatID, const char *text)
  {
    if (ImGui::Selectable(formatID))
      ImGui::SetClipboardText(text);
    ImGui::SameLine();
    ImGui::Text(text);
  }
};

namespace cse::extensions {

ColorPicker::ColorPicker()
{
  cse::addCommand({
    "colorpicker",
    "a colorpicker",
    { /* no arguments */ },
    [](const auto &args) {
      graphics::createWindow(std::make_shared<ColorPickerWindow>());
    }
  });
}

ColorPicker::~ColorPicker()
{
}

}
