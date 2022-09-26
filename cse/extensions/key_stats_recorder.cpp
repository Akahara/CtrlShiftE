#include "key_stats_recorder.h"

#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <filesystem>
#include <time.h>

namespace fs = std::filesystem;

KeyStatsRecorder::KeyStatsRecorder()
{
  std::string sessionFilesDir = cse::getUserFilesPath() + "\\key_records";
  fs::create_directories(sessionFilesDir);
  std::time_t time = std::time(nullptr);
  std::tm currentDate;
  localtime_s(&currentDate, &time);
  std::stringstream sessionFileBase;
  sessionFileBase
    << sessionFilesDir << "\\"
    << (currentDate.tm_year + 1900)
    << "_" << (currentDate.tm_mon + 1)
    << "_" << currentDate.tm_mday
    << "_";

  size_t sessionDayIndex = 0;
  std::string sessionFileName;
  do {
    sessionFileName = sessionFileBase.str() + std::to_string(sessionDayIndex) + ".txt";
    sessionDayIndex++;
  } while (fs::exists(sessionFileName));

  m_sessionFile = std::ofstream(sessionFileName);
  m_currentEventCount = 0;

  // write file header
  m_sessionFile
    << "v1\n"
    << "KeyCode Flags PressTime"
    << std::endl;
}

KeyStatsRecorder::~KeyStatsRecorder()
{
  dumpRecordedEvents();
}

void KeyStatsRecorder::dumpRecordedEvents()
{
  cse::logInfo("Dumping global key events");
  for (size_t i = 0; i < m_currentEventCount; i++) {
    GlobalKeyEvent ev = m_recordedEvents[i];
    m_sessionFile << (long)ev.keyCode << " " << (long)ev.keyFlags << " " << ev.pressTime << "\n";
  }
  m_sessionFile.flush();
  m_currentEventCount = 0;
}

void KeyStatsRecorder::onKeyPressed(GlobalKeyEvent ev)
{
  if (m_currentEventCount == m_recordedEvents.size())
    dumpRecordedEvents();
  m_recordedEvents[m_currentEventCount++] = ev;
}
