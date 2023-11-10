#include "keyswaps.h"

#include "color_picker.h"
#include "universal_shortcut.h"
#include "../../imgui/imgui_internal.h"

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
  ActiveKeyStroke stroke{ ev.keyCode, ev.scanCode };
  if (ev.keyPress == PressType_Press)
    m_activeKeys->insert(stroke);
  else if(ev.keyPress == PressType_Release)
    m_activeKeys->erase(stroke);

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

KeysUtilityWindow::KeysUtilityWindow(const std::unordered_set<ActiveKeyStroke, ActiveKeyStrokeHash> *activeKeys)
  : WindowProcess("Keys")
  , m_activeKeys(activeKeys)
{
}

void KeysUtilityWindow::render()
{
  ImGui::BeginTable("", 3);
  auto column = [](const char *text) { ImGui::TableSetupColumn(text, ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize(text).x); };
  column("ScanCode");
  column("KeyCode");
  column("KeyName\n(layout dependent)");
  ImGui::TableHeadersRow();
  for(const ActiveKeyStroke &stroke : *m_activeKeys)
  {
    if(!m_keynames.contains(stroke.scanCode))
    {
      KeyName nameBuf;
      if(!GetKeyNameTextA(stroke.scanCode << 16, nameBuf.data(), static_cast<int>(nameBuf.size())))
        std::ranges::copy("unknown", nameBuf.data());
      m_keynames.emplace(stroke.scanCode, nameBuf);
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%d 0x%X", stroke.scanCode, stroke.scanCode);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d 0x%X", stroke.keyCode, stroke.keyCode);
    ImGui::TableSetColumnIndex(2);
    ImGui::TextUnformatted(m_keynames[stroke.scanCode].data());
  }
  ImGui::EndTable();
}

KeySwaps::KeySwaps()
{
  cse::keys::addGlobalySuppressedKeystroke(KeySwap::KEYSTROKE_GREATER_OR_LESS);
  cse::keys::addGlobalKeyListener(std::make_shared<KeySwap>());
  cse::keys::addGlobalKeyListener(std::make_shared<KeyRecorder>(&m_activeKeys));

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

  cse::commands::addCommand({
    "keys",
    "see keycodes and scancodes",
    { /* no parameters */ },
    [this](auto &parts) { cse::graphics::createWindow(std::make_shared<KeysUtilityWindow>(&m_activeKeys)); }
  });
}

}
