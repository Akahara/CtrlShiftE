#include "keyswaps.h"

#include "universal_shortcut.h"

namespace cse::extensions
{

struct RepeatableInputEvent
{
  virtual void send() = 0;
  virtual bool canEndMacroSequence() { return true; }
};

struct RepeatableKeyboardEvent : public RepeatableInputEvent
{
  unsigned char keyCode = 0, scanCode = 0;
  PressType pressType = PressType_Release;

  virtual void send() override
  {
    GlobalKeyEvent ev{};
    ev.keyCode = keyCode;
    ev.scanCode = scanCode;
    ev.keyPress = pressType;
    cse::keys::sendKey(ev);
  }

  virtual bool canEndMacroSequence() override
  {
    return pressType == PressType_Release;
  }
};

struct RepeatableMouseEvent : public RepeatableInputEvent
{
  int button = 0;
  long cursorX = 0, cursorY = 0;
  bool isPress = false;

  virtual void send() override
  {
    GlobalButtonEvent ev{};
    ev.button = button;
    ev.cursorX = cursorX;
    ev.cursorY = cursorY;
    ev.isPressed = isPress;
    cse::keys::sendButton(ev);
  }
};

class KeySwap : public GlobalKeyListener
{
public:
  static constexpr GlobalKeystroke KEYSTROKE_GREATER_OR_LESS{ 222, KeyFlags_None };

  virtual void onKeyPressed(const GlobalKeyEvent &ev) override
  {
    if (KEYSTROKE_GREATER_OR_LESS.match(ev) && ev.keyPress != PressType_Release) {
      cse::keys::sendKeyImmediate(0x56, 0xE2); // send < or > based on the current state of the shift key
    }
  }

  virtual void onButtonPressed(const GlobalButtonEvent &ev) override
  {
  }
};

class KeyRecorder : public GlobalKeyListener
{
private:
  std::vector<std::unique_ptr<RepeatableInputEvent>> m_recording;
  std::string m_recordName;

public:
  void startRecording(const std::string &recordName)
  {
    m_recordName = recordName;
  }

  void playRecord()
  {
    for (const auto &input : m_recording)
      input->send();
  }

  virtual void onKeyPressed(const GlobalKeyEvent &ev) override
  {
    if (UniversalShortcut::KEYSTROKE.match(ev)) {
      auto e = std::find_if(m_recording.rbegin(), m_recording.rend(), [](const auto &ev) { return ev->canEndMacroSequence(); });
      m_recording.erase(e.base(), m_recording.end());
      m_recordName.clear();
    } else if(!m_recordName.empty()) {
      auto rev = std::make_unique<RepeatableKeyboardEvent>();
      rev->keyCode = ev.keyCode;
      rev->pressType = ev.keyPress;
      rev->scanCode = ev.scanCode;
      m_recording.push_back(std::move(rev));
    }
  }

  virtual void onButtonPressed(const GlobalButtonEvent &ev) override
  {
    if (!m_recordName.empty()) {
      auto rev = std::make_unique<RepeatableMouseEvent>();
      rev->button = ev.button;
      rev->cursorX = ev.cursorX;
      rev->cursorY = ev.cursorY;
      rev->isPress = ev.isPressed;
      m_recording.push_back(std::move(rev));
    }
  }
};



KeySwaps::KeySwaps()
{
  cse::keys::addGlobalySuppressedKeystroke(KeySwap::KEYSTROKE_GREATER_OR_LESS);
  cse::keys::addGlobalKeyListener(std::make_shared<KeySwap>());

  auto recorder = std::make_shared<KeyRecorder>();

#if 0 // TODO not quite ready yet, macro save files are not implemented and mouse inputs are wanky
  Command recordCmd{
    "macrorec",
    "record a macro",
    { new CommandSaveFilePart("name", "macro") },
    [recorder](auto &parts) { recorder->startRecording(std::string(parts[0])); }
  };
  Command playCmd{
    "macro",
    "play a macro",
    { new CommandSaveFilePart("name", "macro", false||true) },
    [recorder](auto &parts) { recorder->playRecord(); }
  };

  cse::keys::addGlobalKeyListener(recorder);
  cse::addCommand(std::move(playCmd));
  cse::addCommand(std::move(recordCmd));
#endif
}

KeySwaps::~KeySwaps()
{
}

}
