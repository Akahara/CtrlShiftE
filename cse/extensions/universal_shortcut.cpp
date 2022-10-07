#include "universal_shortcut.h"

#include <sstream>
#include <iostream>
#include <algorithm>
#include <tuple>

#include "../../imgui/imgui_internal.h"

#include "../cse_internal.h"
#include "../graphics.h"

class UniversalShortcutBringer : public GlobalKeyListener {
public:
  static constexpr GlobalKeystroke KEYSTROKE{ 'E', KeyFlags_Ctrl | KeyFlags_Shift };

  void onKeyPressed(GlobalKeyEvent ev);
  void onButtonPressed(GlobalButtonEvent ev);
};

class UniversalShortcutWindow : public WindowProcess {
private:
  static constexpr size_t        MAX_INPUT_SIZE = 64;
  char                           m_currentInput[MAX_INPUT_SIZE + 1];
  int                            m_displayFrame;
  size_t                         m_selectedCompletionIndex;
  Command                       *m_currentCommand;
  std::vector<CommandCompletion> m_currentCompletions;
  std::vector<std::string_view>  m_currentInputParts;
  std::vector<bool>              m_currentInputPartsValidState;
  struct {
    int position = 0;
    int selectionStart = 0;
    int selectionEnd = 0;
  } m_carret;
public:
  UniversalShortcutWindow();
  bool beginWindow() override;
  void render() override;
private:
  int onSpecialKey(ImGuiInputTextCallbackData *data);
  void updateCompletions();
  bool runSelectedCommand();
};



void UniversalShortcutBringer::onKeyPressed(GlobalKeyEvent ev)
{
  if (KEYSTROKE.match(ev))
    graphics::createWindow(std::make_shared<UniversalShortcutWindow>());
}

void UniversalShortcutBringer::onButtonPressed(GlobalButtonEvent ev)
{
}

UniversalShortcutWindow::UniversalShortcutWindow()
  : WindowProcess("CtrlShiftE"),
  m_displayFrame(0),
  m_selectedCompletionIndex(-1),
  m_currentInput("")
{
  updateCompletions();
}

bool UniversalShortcutWindow::beginWindow()
{
  if (!m_isVisible)
    return false;
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiViewportFlags_NoTaskBarIcon | ImGuiWindowFlags_NoBackground;
  ImGui::SetNextWindowSize({ 400, 200 });
  return ImGui::Begin(m_windowName.c_str(), &m_isVisible, flags);
}

void UniversalShortcutWindow::render()
{
  // grab focus on frame 2
  if (m_displayFrame == 2 || m_displayFrame == 3) {
    ImGui::GetCurrentContext()->PlatformIO.Platform_SetWindowFocus(ImGui::GetCurrentContext()->CurrentViewport);
    ImGui::SetWindowFocus();

  // if focus is lost, close this window
  } else if (m_displayFrame > 3) {
    bool hasFocus = ImGui::GetCurrentContext()->PlatformIO.Platform_GetWindowFocus(ImGui::GetCurrentContext()->CurrentViewport);
    if (!hasFocus) {
      m_isVisible = false;
      return;
    }
  }
  m_displayFrame++;

  // process text input (no display yet)
  ImGui::SetNextItemWidth(0.f);
  if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
    ImGui::SetKeyboardFocusHere();
  ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackAlways;
  auto callback = [](auto *cd) { return static_cast<UniversalShortcutWindow *>(cd->UserData)->onSpecialKey(cd); };
  if (ImGui::InputText("##Input", m_currentInput, MAX_INPUT_SIZE, flags, callback, this)) {
    updateCompletions();
  }

  // actually display the current input

  // draw cursor and selection
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImVec2 windowOrigin = ImGui::GetWindowPos(); windowOrigin.y += 3.f;
  ImVec2 windowSize = ImGui::GetWindowSize();
  ImVec2 cbox = ImGui::CalcTextSize(m_currentInput, m_currentInput + m_carret.position);
  //drawList->AddRectFilled(windowOrigin, { windowOrigin.x + windowSize.x, windowOrigin.y + windowSize.y }, IM_COL32(255, 255, 255, 255));
  drawList->AddLine({ windowOrigin.x + cbox.x + 1.f, windowOrigin.y }, { windowOrigin.x + cbox.x + 1.f, windowOrigin.y + cbox.y }, IM_COL32_WHITE);
  if (m_carret.selectionEnd != m_carret.selectionStart) {
    int smin = std::min(m_carret.selectionStart, m_carret.selectionEnd);
    int smax = std::max(m_carret.selectionStart, m_carret.selectionEnd);
    ImVec2 selectionStartPos = ImGui::CalcTextSize(m_currentInput, m_currentInput + smin);
    ImVec2 selectionBox = ImGui::CalcTextSize(m_currentInput + smin, m_currentInput + smax);
    drawList->AddRectFilled({ windowOrigin.x + selectionStartPos.x, windowOrigin.y }, { windowOrigin.x + selectionStartPos.x + selectionBox.x, windowOrigin.y + selectionBox.y }, IM_COL32(255, 255, 255, 90));
  }

  // draw text
  size_t partIndex = 0;
  for (size_t i = 0; i < MAX_INPUT_SIZE && m_currentInput[i] != '\0'; i++) {
    bool isPartValid = partIndex < m_currentInputPartsValidState.size() && m_currentInputPartsValidState[partIndex];
    char c = m_currentInput[i];
    ImGui::SameLine(0, 0);
    ImGui::PushStyleColor(ImGuiCol_Text, isPartValid ? IM_COL32_WHITE : IM_COL32(255, 100, 100, 255));
    ImGui::Text("%c", c);
    ImGui::PopStyleColor();
    if(c == ' ')
      partIndex++;
  }

  // draw remaining parts placeholders
  if (m_currentCommand != nullptr && m_currentInputParts.size() < m_currentCommand->parts.size()) {
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 100));
    ImGui::SameLine(0, 0);
    for (size_t i = m_currentInputParts.size(); i < m_currentCommand->parts.size(); i++) {
      ImGui::Text("<%s> ", m_currentCommand->parts[i]->getPlaceHolder().c_str());
    }
    ImGui::PopStyleColor();
  }

  // draw completions
  size_t maxCompletionLength = 0;
  for (CommandCompletion &c : m_currentCompletions)
    maxCompletionLength = std::max(maxCompletionLength, c.missingPart.size());
  constexpr size_t MAX_INFO_PADDING = 8;
  maxCompletionLength = std::min(maxCompletionLength, MAX_INFO_PADDING); // do not add more than 8 spaces

  for (size_t i = 0; i < m_currentCompletions.size(); i++) {
    CommandCompletion &completion = m_currentCompletions[i];
    ImGui::PushStyleColor(ImGuiCol_Text, m_selectedCompletionIndex == i ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 180, 255));
    ImGui::Text("%s%s", m_currentInput, completion.missingPart.c_str());
    if (!completion.info.empty()) {
      ImGui::SameLine(0, 0);
      size_t padding = completion.missingPart.size() > MAX_INFO_PADDING ? 0 : maxCompletionLength - completion.missingPart.size();
      char buf[MAX_INFO_PADDING] = { 0 };
      while (padding--)
        buf[padding] = ' ';
      ImGui::Text("%s", buf); // dirty solution to left justify tooltips
      ImGui::SameLine(0, 0);
      ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
      ImGui::Text(" (%s)", completion.info.c_str());
      ImGui::PopStyleColor();
    }
    ImGui::PopStyleColor();
  }

  // on <enter>, run the active command and close this window
  if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
    if(runSelectedCommand())
      setVisible(false);
  }
  // on <esc>, close this window
  if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    setVisible(false);

}

int UniversalShortcutWindow::onSpecialKey(ImGuiInputTextCallbackData *data)
{
  if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
    // auto completion
    if (m_selectedCompletionIndex >= m_currentCompletions.size())
      return 1;
    std::string &completion = m_currentCompletions[m_selectedCompletionIndex].missingPart;
    data->InsertChars(data->BufTextLen, completion.c_str());
    bool finishedCurrentPart = true;
    if (!completion.empty()) {
      for (size_t i = 0; i < m_currentCompletions.size(); i++) {
        if (i != m_selectedCompletionIndex && m_currentCompletions[i].missingPart.starts_with(completion)) {
          finishedCurrentPart = false; // another longer completion exists
          break;
        }
      }
    }
    if(finishedCurrentPart)
      data->InsertChars(data->BufTextLen, " ");

  } else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
    // selected command
    if (data->EventKey == ImGuiKey_DownArrow) {
      if (m_selectedCompletionIndex < m_currentCompletions.size() - 1)
        m_selectedCompletionIndex++;
    } else if (data->EventKey == ImGuiKey_UpArrow) {
      if (m_selectedCompletionIndex > 0)
        m_selectedCompletionIndex--;
    }
  }

  m_carret.position = data->CursorPos;
  m_carret.selectionStart = data->SelectionStart;
  m_carret.selectionEnd = data->SelectionEnd;
  return 1;
}

static Command *getCSECommand(std::string_view prefix)
{
  for (auto &cmd : cse::getCommands()) {
    if (cmd.prefix == prefix)
      return &cmd;
  }
  return nullptr;
}

void UniversalShortcutWindow::updateCompletions()
{
  m_currentCompletions.clear();
  m_currentInputParts.clear();
  m_currentInputPartsValidState.clear();
  m_selectedCompletionIndex = 0;
  m_currentCommand = nullptr;

  std::string_view currentInput{ m_currentInput };
  size_t spaceIdx = currentInput.find(' ');

  if (spaceIdx == -1) {
    // no command matching, autocomplete with prefix
    bool isValidCmd = false;
    for (auto &cmd : cse::getCommands()) {
      if (cmd.prefix.starts_with(currentInput)) {
        m_currentCompletions.push_back({ cmd.prefix.substr(currentInput.size()), 1.f, cmd.tooltip });
        if (cmd.prefix.size() == currentInput.size())
          isValidCmd = true;
      }
    }
    m_currentInputPartsValidState.push_back(isValidCmd);
    return;
  }

  std::string_view currentCommandPrefix = currentInput.substr(0, spaceIdx);
  m_currentCommand = getCSECommand(currentCommandPrefix);

  m_currentInputPartsValidState.push_back(m_currentCommand != nullptr);
  if (m_currentCommand == nullptr)
    return; // no matching command

  for (CommandPart *part : m_currentCommand->parts)
    part->updateState();

  // check all parts valid state
  int partIndex = 0;
  size_t lastSplit = spaceIdx + 1;
  for (size_t i = lastSplit; i <= currentInput.length(); i++) {
    if (i == currentInput.size() || currentInput[i] == ' ') {
      if (lastSplit < i) {
        if (partIndex >= static_cast<int>(m_currentCommand->parts.size())) {
          m_currentInputPartsValidState.push_back(false); // command too long
          m_currentInputParts.push_back("");
        } else {
          std::string_view part = currentInput.substr(lastSplit, i - lastSplit);
          m_currentInputPartsValidState.push_back(m_currentCommand->parts[partIndex]->isGood(part));
          m_currentInputParts.push_back(part);
          partIndex++;
        }
      }
      lastSplit = i + 1;
    }
  }

  // check the last part for completions
  std::string_view remainingInputPart = currentInput.substr(currentInput.find_last_of(' ')+1);
  size_t lastPartIndex = std::count(currentInput.begin(), currentInput.end(), ' ') - 1;

  if (lastPartIndex < m_currentCommand->parts.size())
    m_currentCommand->parts[lastPartIndex]->getCompletions(remainingInputPart, m_currentCompletions);

  std::sort(m_currentCompletions.begin(), m_currentCompletions.end(), [](auto &c1, auto &c2) { return c1.relevanceScore > c2.relevanceScore; });
}

bool UniversalShortcutWindow::runSelectedCommand()
{
  if (m_currentCommand == nullptr)
    return false;

  if (m_currentInputParts.size() != m_currentCommand->parts.size())
    return false;
  if (std::find(m_currentInputPartsValidState.begin(), m_currentInputPartsValidState.end(), false) != m_currentInputPartsValidState.end())
    return false;

  // actually run the command
  m_currentCommand->executor(m_currentInputParts);
  return true;
}



namespace cse::extensions {

UniversalShortcut::UniversalShortcut()
{
  cse::keys::addGlobalySuppressedKeystroke(UniversalShortcutBringer::KEYSTROKE);
  cse::keys::addGlobalKeyListener(std::make_shared<UniversalShortcutBringer>());
}

UniversalShortcut::~UniversalShortcut()
{
}

}