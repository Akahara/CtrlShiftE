#pragma once

#include "../cse.h"

class UniversalShortcutBringer : public GlobalKeyListener {
public:
  static constexpr GlobalKeystroke KEYSTROKE{ 'E', KeyFlags_Ctrl | KeyFlags_Shift };

  void onKeyPressed(GlobalKeyEvent ev);
};

class UniversalShortcutWindow : public WindowProcess {
private:
  static constexpr size_t  MAX_INPUT_SIZE = 64;
  char                     m_currentInput[MAX_INPUT_SIZE];
  int                      m_displayFrame;
  size_t                   m_selectedCommandIndex;
  std::vector<IOTCommand *> m_currentCommands;
public:
  UniversalShortcutWindow();
  bool beginWindow() override;
  void render() override;
private:
  static int onUpDownKeyDispatch(ImGuiInputTextCallbackData *data);
  int onUpDownKey(ImGuiInputTextCallbackData *data);
  void updateCommands();
  void runSelectedCommand();
};