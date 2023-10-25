#pragma once

#include <memory>
#include <string>

#include "../imgui/imgui.h"

class WindowProcess {
protected:
  std::string m_windowName;
  bool m_isVisible = true;

public:
  explicit WindowProcess(std::string_view windowName);
  virtual ~WindowProcess() = default;

  virtual bool beginWindow();
  virtual void render() = 0;

  const std::string &getName() const { return m_windowName; }
  bool isVisible() const { return m_isVisible; }
  void setVisible(bool visible) { m_isVisible = visible; }
};

namespace cse::graphics {

#ifdef CSE_EXPOSE_INTERNALS
struct Window {
  std::shared_ptr<WindowProcess> process;
};

void loadGraphics();
void destroyGraphics();
void closeAllWindows();
#endif

void render();
bool shouldDispose();

void createWindow(const std::shared_ptr<WindowProcess> &process);

void prepareAlwaysOnTop();
void prepareTransparent(bool *cond);

}