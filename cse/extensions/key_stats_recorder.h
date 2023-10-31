#pragma once

#include "cse.h"

#include <unordered_map>

struct ClickPos {
  int x, y;
};

bool operator==(const ClickPos &p1, const ClickPos &p2);

template<>
struct std::hash<ClickPos> {
  size_t operator()(const ClickPos &p) const noexcept;
};


namespace cse::extensions {

/*
Design note: to record button presses we cannot simply have a 2d array of fixed size because
the user may add/remove a secondary monitor at runtime, and click received from secondary
monitors are in a larger range (maybe even negative).
Instead
*/

class KeyListener : public GlobalKeyListener {
public:
  KeyListener();
  ~KeyListener() override;

  void onKeyPressed(const GlobalKeyEvent &ev) override;
  void onButtonPressed(const GlobalButtonEvent &ev) override;

private:
  void dumpRecordedEvents();

private:
  constexpr static size_t RECORDED_SCANCODE_COUNT = 100;  // record only the first 100 scan codes
  constexpr static size_t SAVE_PERIOD = 500;              // save the file every N events
  constexpr static int    BUTTON_POSITION_THRESHOLD = 40; // do not differenciate button pressed that are less than N pixels appart

  std::unordered_map<ClickPos, unsigned int> m_buttonPresses;
  unsigned int m_keyPresses[RECORDED_SCANCODE_COUNT];
  int          m_minButtonX, m_minButtonY, m_maxButtonX, m_maxButtonY;
  size_t       m_currentEventCount;
  fs::path     m_sessionFilePath;
  long long    m_firstTime;
};

class KeyStatsRecorder : public CSEExtension {
public:
  static constexpr const char *EXTENSION_NAME = "Key Stats Recorder";

  KeyStatsRecorder();
};

}