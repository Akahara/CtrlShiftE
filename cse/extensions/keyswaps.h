#pragma once

#include "universal_shortcut.h"
#include "../cse.h"

namespace cse::extensions {

struct RepeatableInputEvent {
  virtual ~RepeatableInputEvent() = default;
  virtual void send() = 0;
  virtual bool canEndMacroSequence() { return true; }
};

struct RepeatableKeyboardEvent : RepeatableInputEvent {
  unsigned char keyCode = 0, scanCode = 0;
  PressType pressType = PressType_Release;

  void send() override;
  bool canEndMacroSequence() override;
};

struct RepeatableMouseEvent : RepeatableInputEvent {
  int button = 0;
  long cursorX = 0, cursorY = 0;
  bool isPress = false;

  void send() override;
};

class KeySwap : public GlobalKeyListener {
public:
  static constexpr GlobalKeystroke KEYSTROKE_GREATER_OR_LESS{ 222, KeyFlags_None };

  void onKeyPressed(const GlobalKeyEvent &ev) override;
  void onButtonPressed(const GlobalButtonEvent &ev) override {}
};

class KeyRecorder : public GlobalKeyListener {
public:
  void startRecording(const std::string &recordName);
  void playRecord() const;

  void onKeyPressed(const GlobalKeyEvent &ev) override;
  void onButtonPressed(const GlobalButtonEvent &ev) override;

private:
  std::vector<std::unique_ptr<RepeatableInputEvent>> m_recording;
  std::string m_recordName;
};

class KeySwaps : public CSEExtension {
public:
	KeySwaps();
};

}