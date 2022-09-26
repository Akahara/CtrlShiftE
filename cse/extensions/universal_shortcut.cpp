#include "universal_shortcuts.h"

#include <sstream>
#include <iostream>
#include <algorithm>

#include "../../imgui/imgui_internal.h"

#include "../cse_internal.h"
#include "../graphics.h"

void UniversalShortcutBringer::onKeyPressed(GlobalKeyEvent ev)
{
  if (KEYSTROKE.match(ev))
    graphics::createWindow(std::make_shared<UniversalShortcutWindow>());
}

UniversalShortcutWindow::UniversalShortcutWindow()
  : WindowProcess("CtrlShiftE"),
  m_displayFrame(0),
  m_selectedCommandIndex(-1),
  //m_selectedCommand(),
  m_currentInput("")
{
  updateCommands();
}

bool UniversalShortcutWindow::beginWindow()
{
  return ImGui::Begin(m_windowName.c_str(), &m_isVisible, ImGuiWindowFlags_NoDecoration);
}

void UniversalShortcutWindow::render()
{
  // grab focus on frame 2
  if (m_displayFrame == 2) {
    ImGui::GetCurrentContext()->PlatformIO.Platform_SetWindowFocus(ImGui::GetCurrentContext()->CurrentViewport);
    ImGui::SetWindowFocus();

  // if focus is lost, close this window
  } else if (m_displayFrame > 2) {
    bool hasFocus = ImGui::GetCurrentContext()->PlatformIO.Platform_GetWindowFocus(ImGui::GetCurrentContext()->CurrentViewport);
    if (!hasFocus) {
      m_isVisible = false;
      return;
    }
  }
  m_displayFrame++;

  ImGui::PushItemWidth(-1);
  if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
    ImGui::SetKeyboardFocusHere();
  ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackHistory;
  if(ImGui::InputText("##Input", m_currentInput, MAX_INPUT_SIZE, flags, onUpDownKeyDispatch, this))
    updateCommands();
  ImGui::PopItemWidth();

  for (size_t i = 0; i < m_currentCommands.size(); i++) {
    ImGui::BeginDisabled(i != m_selectedCommandIndex);
    ImGui::Text(m_currentCommands[i]->getText().c_str());
    ImGui::EndDisabled();
  }

  // on <enter>, run the active command and close this window
  if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !m_currentCommands.empty()) {
    runSelectedCommand();
    setVisible(false);
  }
  // on <esc>, close this window
  if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    setVisible(false);

}

int UniversalShortcutWindow::onUpDownKeyDispatch(ImGuiInputTextCallbackData *data)
{
  return static_cast<UniversalShortcutWindow *>(data->UserData)->onUpDownKey(data);
}

int UniversalShortcutWindow::onUpDownKey(ImGuiInputTextCallbackData *data)
{
  if (data->EventKey == ImGuiKey_DownArrow) {
    if (m_selectedCommandIndex < m_currentCommands.size() - 1)
      m_selectedCommandIndex++;
  } else if (data->EventKey == ImGuiKey_UpArrow) {
    if (m_selectedCommandIndex > 0)
      m_selectedCommandIndex--;
  }
  return 1;
}

void UniversalShortcutWindow::updateCommands()
{
  for (IOTCommand *command : m_currentCommands)
    delete command;
  std::vector<IOTCommand*> newCommands;
  std::string currentInput{ m_currentInput };
  for (auto &generator : cse::getIOTGenerators())
    generator->getCommands(currentInput, newCommands);
  m_selectedCommandIndex = 0;
  std::sort(newCommands.begin(), newCommands.end(), [](auto c1, auto c2) { return c1->getRelevanceScore() > c2->getRelevanceScore(); });
  m_currentCommands = std::move(newCommands);
}

void UniversalShortcutWindow::runSelectedCommand()
{
  assert(m_selectedCommandIndex < m_currentCommands.size());
  m_currentCommands[m_selectedCommandIndex]->runCommand();
}
