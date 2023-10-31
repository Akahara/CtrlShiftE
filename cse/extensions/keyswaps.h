#pragma once

#include <unordered_set>
#include <array>

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

struct ActiveKeyStroke {
  unsigned char keyCode, scanCode;
};

struct ActiveKeyStrokeHash {
  size_t operator()(const ActiveKeyStroke &stroke) const noexcept
  {
    return stroke.keyCode | (stroke.scanCode << sizeof(stroke.keyCode));
  }
};

inline bool operator==(const ActiveKeyStroke &k1, const ActiveKeyStroke &k2)
{
  return k1.keyCode == k2.keyCode && k1.scanCode == k2.scanCode;
}

class KeyRecorder : public GlobalKeyListener {
public:
  explicit KeyRecorder(std::unordered_set<ActiveKeyStroke, ActiveKeyStrokeHash> *activeKeys)
    : m_activeKeys(activeKeys) {}

  void startRecording(const std::string &recordName);
  void playRecord() const;

  void onKeyPressed(const GlobalKeyEvent &ev) override;
  void onButtonPressed(const GlobalButtonEvent &ev) override;

private:
  std::vector<std::unique_ptr<RepeatableInputEvent>> m_recording;
  std::string m_recordName;
  std::unordered_set<ActiveKeyStroke, ActiveKeyStrokeHash> *m_activeKeys;
};

class KeysUtilityWindow : public WindowProcess
{
public:
  explicit KeysUtilityWindow(const std::unordered_set<ActiveKeyStroke, ActiveKeyStrokeHash> *activeKeys);

  void render() override;

private:
  using KeyName = std::array<char, 16>;
  const std::unordered_set<ActiveKeyStroke, ActiveKeyStrokeHash> *m_activeKeys;
  std::unordered_map<unsigned char, KeyName>                      m_keynames;
};

class KeySwaps : public CSEExtension {
public:
  static constexpr const char *EXTENSION_NAME = "Key Swaps";

  KeySwaps();

private:
  std::unordered_set<ActiveKeyStroke, ActiveKeyStrokeHash> m_activeKeys;
};

}
