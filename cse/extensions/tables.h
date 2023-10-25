#pragma once

#include <unordered_map>

#include "cse.h"

namespace cse::extensions {

using WindowProvider = std::function<WindowProcess *()>;

class ASCIITableWindow : public WindowProcess {
public:
  ASCIITableWindow();

  void render() override;
};


class GLSLTableWindow : public WindowProcess {
public:
  GLSLTableWindow();

  void render() override;
};

class ColorsSchemesWindow : public WindowProcess {
public:
  ColorsSchemesWindow()
    : WindowProcess("Color schemes")
  {
  }

  void render() override;

  std::string formatColorStr(int color);

  static void replaceFirst(std::string &toAffect, const char *toReplace, const std::string &replacement);

  void drawColorScheme(const int *scheme);
  void drawColorBtn(int color);
  void drawAnsiBtn(const char *colorName, int foregroundCode) const;
  void drawAnsiBtn(const char *colorName, int foregroundCode, int color) const;

private:
  char m_userFormat[32] = "rgb($r,$g,$b)";
  int  m_ansiCSI = 0;
};

class Tables : public CSEExtension {
public:
  Tables();

private:
  std::unordered_map<const char*, WindowProvider> m_tables;
};

}
