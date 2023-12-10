#include "regex.h"

#include <ranges>
#include <regex>

#include "cse_commands.h"

namespace cse::extensions
{
Regex::Regex()
{
  cse::commands::addCommand({
    "regex",
    "regex substitution & matching",
    { /* no arguments */ },
    [](auto &args) { cse::graphics::createWindow(std::make_shared<RegexWindow>()); }
  });
}
  
RegexWindow::RegexWindow()
  : WindowProcess("Regex")
{
  json previousSession = cse::extensions::getUserGlobalConfig("regex");
  m_textInput = previousSession.value("input", "");
  m_regexInput = previousSession.value("regex", "");
  updateOutputNow(m_regexInput, m_textInput);
}

RegexWindow::~RegexWindow()
{
  cse::extensions::updateUserGlobalConfig("regex", {
    { "input", m_textInput },
    { "regex", m_regexInput },
  });
}

void RegexWindow::render()
{
  
  ImGui::BeginChild("r", { -1,0 }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY);
  if (ImGui::InputTextMultiline("###r", &m_regexInput, { -1,-1 })) updateOutput(m_regexInput, m_textInput);
  ImGui::EndChild();
  ImGui::BeginChild("i", { -1,0 }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY);
  if (ImGui::InputTextMultiline("###i", &m_textInput, { -1,-1 })) updateOutput(m_regexInput, m_textInput);
  ImGui::EndChild();
  {
    std::lock_guard _{ m_textOutputMutex };
    ImGui::InputTextMultiline("###o", &m_textOutput, {-1,-1}, ImGuiInputTextFlags_ReadOnly);
  }
}

void RegexWindow::updateOutputNow(std::string regex, std::string text)
{
  text = runMultiSubstitution(regex, text);
  std::lock_guard _{ m_textOutputMutex };
  m_textOutput = std::move(text);
}

std::string RegexWindow::runMultiSubstitution(const std::string &regex, std::string text) try
{
  std::vector<std::pair<std::regex, std::string>> compiledRegexes;

  for (auto line : regex 
    | std::views::split('\n')
    | std::views::transform([](auto l) { return std::string_view{ l }; })
    | std::views::filter([](auto l) { return !l.empty(); }))
  {
    size_t split = line.find(" -> ");
    if (split != std::string::npos) {
      compiledRegexes.emplace_back(std::regex{ line.begin(), line.begin() + split }, unescape(line.substr(split+4)));
    } else {
      compiledRegexes.emplace_back(std::regex{ line.begin(), line.end() }, std::string{});
    }
  }

  for (auto &[re, repl] : compiledRegexes)
    text = std::regex_replace(text, re, repl);
  return text;
} catch (const std::regex_error &e) {
  return e.what();
}

std::string RegexWindow::unescape(std::string_view text)
{
  size_t prev = 0;
  size_t backslash = text.find('\\');
  if (backslash == std::string_view::npos) return std::string(text);

  std::stringstream ss;

  while(true) {
    ss << text.substr(prev, backslash - prev);
    if (backslash >= text.size()-1) break;

    ss << unescapeBackslashCharacter(text[backslash+1]);

    prev = backslash+2;
    backslash = text.find('\\', prev);
  }

  return ss.str();
}

char RegexWindow::unescapeBackslashCharacter(char c)
{
  switch(c) {
  case 'n': return '\n';
  case 't': return '\t';
  case 'r': return '\r';
  case '\\': return '\\';
  default: return '?';
  }
}

}
