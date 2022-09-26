#pragma once

#include <array>
#include <fstream>

#include "../cse.h"

class KeyStatsRecorder : public GlobalKeyListener {
private:
  std::array<GlobalKeyEvent, 128> m_recordedEvents;
  size_t                          m_currentEventCount;
  std::ofstream                   m_sessionFile;
public:
  KeyStatsRecorder();
  ~KeyStatsRecorder();
  void onKeyPressed(GlobalKeyEvent ev) override;
private:
  void dumpRecordedEvents();
};