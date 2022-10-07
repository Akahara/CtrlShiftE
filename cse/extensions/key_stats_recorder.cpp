#include "key_stats_recorder.h"

#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <filesystem>
#include <time.h>
#include <fstream>
#include <array>

namespace fs = std::filesystem;

enum class EventType {
  KEY, BUTTON
};

struct GlobalEvent {
  EventType type;
  union {
    GlobalKeyEvent key;
    GlobalButtonEvent button;
  } ev;
};

class KeyListener : public GlobalKeyListener {
private:
  std::array<GlobalEvent, 128> m_recordedEvents;
  size_t                       m_currentEventCount;
  std::ofstream                m_sessionFile;
public:
  KeyListener();
  ~KeyListener();
  void onKeyPressed(GlobalKeyEvent ev) override;
  void onButtonPressed(GlobalButtonEvent ev) override;
private:
  void dumpRecordedEvents();
};

KeyListener::KeyListener()
{
  fs::path sessionFilesDir = cse::getUserFilesPath() / "key_records";
  fs::create_directories(sessionFilesDir);
  std::time_t time = std::time(nullptr);
  std::tm currentDate;
  localtime_s(&currentDate, &time);
  std::stringstream sessionFileBase;
  sessionFileBase
    << (currentDate.tm_year + 1900)
    << "_" << (currentDate.tm_mon + 1)
    << "_" << currentDate.tm_mday
    << "_";

  size_t sessionDayIndex = 0;
  fs::path sessionFileName;
  do {
    sessionFileName = sessionFilesDir / sessionFileBase.str();
    sessionFileName += std::to_string(sessionDayIndex) + ".txt";
    sessionDayIndex++;
  } while (fs::exists(sessionFileName));

  m_sessionFile = std::ofstream(sessionFileName, std::ios::binary);
  m_currentEventCount = 0;

  // write file header
  m_sessionFile.write("v2", 2);
}

KeyListener::~KeyListener()
{
  dumpRecordedEvents();
}

void KeyListener::dumpRecordedEvents()
{
  cse::logInfo("Dumping global key events");
  for (size_t i = 0; i < m_currentEventCount; i++) {
    GlobalEvent &wrappedEvent = m_recordedEvents[i];
    switch (wrappedEvent.type) {
    case EventType::KEY: {
      GlobalKeyEvent &ev = wrappedEvent.ev.key;
      m_sessionFile.write(&ev.keyCode, 1);
    } break;
    case EventType::BUTTON: {
      GlobalButtonEvent &ev = wrappedEvent.ev.button;
      m_sessionFile.write((char*)&ev.button, 1);
    } break;
    }
  }
  m_sessionFile.flush();
  m_currentEventCount = 0;
}

void KeyListener::onKeyPressed(GlobalKeyEvent ev)
{
  if (m_currentEventCount == m_recordedEvents.size())
    dumpRecordedEvents();
  m_recordedEvents[m_currentEventCount++] = { EventType::KEY, {.key = ev} };
}

void KeyListener::onButtonPressed(GlobalButtonEvent ev)
{
  if (m_currentEventCount == m_recordedEvents.size())
    dumpRecordedEvents();
  m_recordedEvents[m_currentEventCount++] = { EventType::BUTTON, {.button = ev} };
}


namespace cse::extensions {

KeyStatsRecorder::KeyStatsRecorder()
{
  cse::keys::addGlobalKeyListener(std::make_shared<KeyListener>());
}

KeyStatsRecorder::~KeyStatsRecorder()
{
}

}