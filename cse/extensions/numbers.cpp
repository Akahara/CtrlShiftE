#include "numbers.h"

#include <bitset>

#include "cse_commands.h"
#include "expressions/expressions.h"

using namespace std::string_literals;

namespace cse::extensions
{

size_t NumbersWindow::s_windowCount;
std::vector<std::weak_ptr<NumbersWindow>> NumbersWindow::s_allWindows;

NumbersWindow::NumbersWindow(size_t winId)
  : WindowProcess("numbers-#"s + static_cast<char>('A' + winId))
  , m_winVarName("#"s + static_cast<char>('A' + winId))
{}

void NumbersWindow::render()
{
  ImGui::BeginDisabled();
  for(size_t i = 0; i < BIT_COUNT; i++) {
    if (i == BIT_COUNT - (CHAR_BIT * m_enabledByteCount))
      ImGui::EndDisabled();

    int_type bitFlag = static_cast<int_type>(1) << (BIT_COUNT - 1 - i);
    if (ImGui::Button(((m_value & bitFlag ? "1##" : "0##") + std::to_string(i)).c_str())) {
      m_value ^= bitFlag;
      updateTexts();
    }

    if (i % 16 == 15);
    else if (i % 8 == 7)
      ImGui::SameLine();
    else if(i != BIT_COUNT-1)
      ImGui::SameLine(0, 0);
  }

  if(m_textFocus >= 0) {
    ImGui::SetKeyboardFocusHere(m_textFocus);
    m_textFocus = -1;
  }
  if (ImGui::InputText("Unsigned", m_unsignedString, sizeof(m_unsignedString), ImGuiInputTextFlags_EnterReturnsTrue))
  { parseAndEvalExpression(m_unsignedString); m_textFocus = 0; }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("non standard");
  if (ImGui::InputText("Signed", m_signedString, sizeof(m_unsignedString), ImGuiInputTextFlags_EnterReturnsTrue))
  { parseAndEvalExpression(m_signedString); m_textFocus = 1; }
  if (ImGui::InputText("Hexadecimal", m_hexString, sizeof(m_unsignedString), ImGuiInputTextFlags_EnterReturnsTrue))
  { parseAndEvalExpression(m_hexString); m_textFocus = 2; }
  if (ImGui::InputText("Binary", m_binString, sizeof(m_binString), ImGuiInputTextFlags_EnterReturnsTrue))
  { parseAndEvalExpression(m_binString); m_textFocus = 3; }
  if (ImGui::InputText(m_enabledByteCount == 8 ? "Bits as double" : "Bits as float", m_floatString, sizeof(m_floatString),
      ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsScientific)) {
    if(m_enabledByteCount == 8) {
      double x = std::stod(m_floatString);
      m_value = std::bit_cast<int_type>(x);
    } else {
      float x = std::stof(m_floatString);
      m_value = std::bit_cast<int32_t>(x);
    }
    updateTexts();
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("no evaluation, only literals");

  ImGui::TextColored({ 1,0,0,1 }, "%s", m_evalErrorString.c_str());

  for (size_t bc = 1; bc <= 8; bc <<= 1) {
    if (ImGui::Button(std::to_string(bc).c_str())) {
      m_enabledByteCount = bc;
      updateTexts();
    }
    ImGui::SameLine(0, 1.f);
  }

  if (ImGui::Button("New Window"))
    cse::graphics::createWindow(make_window());
  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Button, 0xff46d95c);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xff46d95c);
  ImGui::Button("?");
  ImGui::PopStyleColor(2);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip(
    "Accepted expressions examples: 3+4  5<<0b101  0xF & #A\n"
    "When multiple windows are open, use #A,#B... to use their values");
}

NumbersWindow::int_type NumbersWindow::getValue() const
{
  switch (m_enabledByteCount) {
  case 1: return static_cast<int8_t >(m_value);
  case 2: return static_cast<int16_t>(m_value);
  case 4: return static_cast<int32_t>(m_value);
  case 8: return static_cast<int64_t>(m_value);
  default: return 0; // unreachable
  }
}

template<size_t N>
void copystr(char (&buf)[N], const std::string &text)
{
  size_t i = 0;
  for (; i < N - 1 && i < text.size(); i++)
    buf[i] = text[i];
  buf[i] = '\0';
}

template<class smallint_type, class smallunsigned_type>
void NumbersWindow::updateTexts(smallint_type value, smallunsigned_type uvalue)
{
  std::stringstream ss;

  // binary 0b
  ss << "0b";
  if (!value) {
    ss << "0";
  } else {
    int b = 0;
    while (uvalue >= 1ull << (b+1) && b < 64) b++;
    for (; b >= 0; b--)
      ss << ((uvalue & 1ull << b) ? '1' : '0');
  }
  copystr(m_binString, ss.str()); ss.str("");

  // unsigned 0u (non standard)
  ss << "0u" << uvalue;
  copystr(m_unsignedString, ss.str()); ss.str("");

  // signed
  ss << value;
  copystr(m_signedString, ss.str()); ss.str("");

  // hexadecimal 0x
  ss << "0x" << std::hex << value;
  copystr(m_hexString, ss.str()); ss.str("");

  // float
  if (m_enabledByteCount == 8)
    ss << std::bit_cast<double>(m_value);
  else
    ss << std::bit_cast<float>(static_cast<int32_t>(m_value));
  copystr(m_floatString, ss.str()); ss.str("");
}

void NumbersWindow::updateTexts()
{
  switch (m_enabledByteCount) {
  case 1: updateTexts(static_cast<int>(static_cast<int8_t>(m_value)), static_cast<unsigned int>(static_cast<uint8_t>(m_value))); break;
  case 2: updateTexts(static_cast<int16_t>(m_value), static_cast<uint16_t>(m_value)); break;
  case 4: updateTexts(static_cast<int32_t>(m_value), static_cast<uint32_t>(m_value)); break;
  case 8: updateTexts(static_cast<int64_t>(m_value), static_cast<uint64_t>(m_value)); break;
  default: break;
  }
}

void NumbersWindow::parseAndEvalExpression(const std::string &expression)
{
  expressions::LexingContext lex{ expression };
  expressions::ParsingContext parse{ expressions::lex(lex), expression };
  if (lex.lexingError) { m_evalErrorString = lex.lexingError.value(); return; }
  std::unique_ptr<expressions::Expression> exp = expressions::parse(parse);
  if (parse.parsingError) { m_evalErrorString = parse.parsingError.value(); return; }
  expressions::EvaluationContext eval{};
  std::ranges::for_each(s_allWindows, [&](auto &win) {
    if (auto p = win.lock(); p)
      eval.variables.emplace(p->m_winVarName, p->getValue());
  });
  int64_t val = evalInt(eval, *exp);
  if (eval.evaluationError) { m_evalErrorString = eval.evaluationError.value(); return; }

  m_value = val;
  m_evalErrorString = "";
  updateTexts();
}

Numbers::Numbers()
{
  cse::commands::addCommand({
    "calculator",
    "bring up a n-ary calculator",
    { /* no arguments */ },
    [](auto &args) { cse::graphics::createWindow(NumbersWindow::make_window()); }
  });
}
}
