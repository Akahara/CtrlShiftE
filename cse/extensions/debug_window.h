#pragma once

#include "../cse.h"
#include "../graphics.h"

class DebugWindowProcess : public WindowProcess {
private:
  static constexpr const char *tableName = "Logs";
  bool m_autoScroll = true;
public:
  DebugWindowProcess();

  bool beginWindow() override;
  void render() override;
public:
  static void log(std::string_view line);
};
