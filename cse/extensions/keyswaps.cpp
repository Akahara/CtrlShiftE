#include "keyswaps.h"

#include "universal_shortcut.h"

namespace cse::extensions
{
void RepeatableKeyboardEvent::send()
{
  GlobalKeyEvent ev{};
  ev.keyCode = keyCode;
  ev.scanCode = scanCode;
  ev.keyPress = pressType;
  cse::keys::sendKey(ev);
}

bool RepeatableKeyboardEvent::canEndMacroSequence()
{
  return pressType == PressType_Release;
}

void RepeatableMouseEvent::send()
{
  GlobalButtonEvent ev{};
  ev.button = button;
  ev.cursorX = cursorX;
  ev.cursorY = cursorY;
  ev.isPressed = isPress;
  cse::keys::sendButton(ev);
}

void KeySwap::onKeyPressed(const GlobalKeyEvent& ev)
{
  if (KEYSTROKE_GREATER_OR_LESS.doesStrokeMatch(ev) && ev.keyPress != PressType_Release) {
    cse::keys::sendKeyImmediate(0x56, 0xE2); // send < or > based on the current state of the shift key
  }
}

void KeyRecorder::startRecording(const std::string& recordName)
{
  m_recordName = recordName;
}

void KeyRecorder::playRecord() const
{
  for (const auto &input : m_recording)
    input->send();
}

void KeyRecorder::onKeyPressed(const GlobalKeyEvent& ev)
{
  if (UniversalShortcut::KEYSTROKE.doesStrokeMatch(ev)) {
    auto e = std::find_if(m_recording.rbegin(), m_recording.rend(), [](const auto &ev) { return ev->canEndMacroSequence(); });
    m_recording.erase(e.base(), m_recording.end());
    m_recordName.clear();
  } else if (!m_recordName.empty()) {
    auto rev = std::make_unique<RepeatableKeyboardEvent>();
    rev->keyCode = ev.keyCode;
    rev->pressType = ev.keyPress;
    rev->scanCode = ev.scanCode;
    m_recording.push_back(std::move(rev));
  }
}

void KeyRecorder::onButtonPressed(const GlobalButtonEvent& ev)
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

}
