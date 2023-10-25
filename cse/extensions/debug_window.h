#pragma once

#include "cse.h"

namespace cse::extensions
{

class DebugWindowProcess : public WindowProcess {
public:
  DebugWindowProcess();

  void render() override;

  static void log(std::string_view line);

private:
  static std::vector<std::string> s_logLines;
  bool m_autoScroll = true;
};

}
