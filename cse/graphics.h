#pragma once

#include <string>
#include <memory>

#include "../imgui/imgui.h"

class WindowProcess {
protected:
  std::string m_windowName;
  bool m_isVisible = true;

public:
  WindowProcess(std::string_view windowName);
  virtual ~WindowProcess() = default;

  virtual bool beginWindow();
  virtual void render() = 0;

  const std::string &getName() const { return m_windowName; }
  bool isVisible() const { return m_isVisible; }
  void setVisible(bool visible) { m_isVisible = visible; }
};

namespace graphics {

void loadGraphics();
void destroyGraphics();

void render();
bool shouldDispose();

void createWindow(const std::shared_ptr<WindowProcess> &process);
void closeAllWindows();

namespace window_helper {

void prepareAlwaysOnTop();
void prepareTransparent(bool *cond);

}

}