#pragma once

#include "cse.h"

namespace cse::extensions {

class UniversalShortcutBringer : public GlobalKeyListener {
public:
  void onKeyPressed(const GlobalKeyEvent &ev) override;
  void onButtonPressed(const GlobalButtonEvent &ev) override;
};

class UniversalShortcutWindow : public WindowProcess {
public:
  UniversalShortcutWindow(bool noExit=false);
  bool beginWindow() override;
  void render() override;

private:
  int onSpecialKey(ImGuiInputTextCallbackData *g_globalData);
  void updateCompletions();
  bool runSelectedCommand();

private:
  static constexpr size_t        MAX_INPUT_SIZE = 64;
  char                           m_currentInput[MAX_INPUT_SIZE + 1];
  int                            m_displayFrame;
  size_t                         m_selectedCompletionIndex;
  Command                       *m_currentCommand;
  std::vector<CommandCompletion> m_currentCompletions;
  std::vector<std::string_view>  m_currentInputParts;
  std::vector<bool>              m_currentInputPartsValidState;
  bool                           m_alreadyPrompted = false;
  bool                           m_neverCloseWindow;
  struct {
    int position = 0;
    int selectionStart = 0;
    int selectionEnd = 0;
  } m_carret;
};

class UniversalShortcut : public CSEExtension {
public:
  static constexpr const char *EXTENSION_NAME = "Universal Shortcut";
  static constexpr GlobalKeystroke KEYSTROKE{ 'E', KeyFlags_Ctrl | KeyFlags_Shift };

  UniversalShortcut();
};

}
