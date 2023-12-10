#pragma once

#include "cse_extensions.h"
#include "cse_graphics.h"
#include "utils/debouncing.h"

namespace cse::extensions 
{

class Regex : public CSEExtension
{
public:
  static constexpr const char *EXTENSION_NAME = "Regex";

  Regex();
};

class RegexWindow : public WindowProcess
{
public:
  RegexWindow();
  ~RegexWindow() override;

  void render() override;

private:
  void updateOutputNow(std::string regex, std::string text);
  won::debounced<decltype(&RegexWindow::updateOutputNow)> updateOutput{ &RegexWindow::updateOutputNow, this, std::chrono::seconds{ 0 } };
  std::string runMultiSubstitution(const std::string &regex, std::string text);

  static std::string unescape(std::string_view text);
  static char unescapeBackslashCharacter(char c);

private:
  std::string m_regexInput;
  std::string m_textInput;
  std::string m_textOutput;
  std::mutex m_textOutputMutex;
};

}
