#include "key_stats_recorder.h"

#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <filesystem>
#include <time.h>
#include <fstream>


inline long long currentTimeMillis()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

namespace cse::extensions
{


KeyListener::KeyListener()
  : m_keyPresses()
  , m_minButtonX(std::numeric_limits<int>::max())
  , m_minButtonY(std::numeric_limits<int>::max())
  , m_maxButtonX(std::numeric_limits<int>::min())
  , m_maxButtonY(std::numeric_limits<int>::min())
  , m_currentEventCount(0)
  , m_firstTime(currentTimeMillis())
{
  fs::path sessionFilesDir = cse::extensions::getUserFilesPath() / "key_records";
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
  do {
    m_sessionFilePath = sessionFilesDir / sessionFileBase.str();
    m_sessionFilePath += std::to_string(sessionDayIndex) + ".csv";
    sessionDayIndex++;
  } while (fs::exists(m_sessionFilePath));

  // write the file at least once
  dumpRecordedEvents();
}

KeyListener::~KeyListener()
{
  dumpRecordedEvents();
}

void KeyListener::dumpRecordedEvents()
{
  cse::log("Dumping global key events");
  std::ofstream sessionFile{ m_sessionFilePath, std::ios::binary };

  sessionFile << "v3;\n";

  sessionFile << "first millis;last millis;\n";
  sessionFile << m_firstTime << ";" << currentTimeMillis() << ";\n";

  sessionFile << "presses per scancode up to;" << RECORDED_SCANCODE_COUNT << ";\n";
  for (unsigned int m_keyPresse : m_keyPresses)
    sessionFile << m_keyPresse << ";";
  sessionFile << "\n";

  sessionFile << "pixels per cell;cellMinX;cellMinY;cellMaxX;cellMaxY;\n";
  sessionFile << BUTTON_POSITION_THRESHOLD << ";" << m_minButtonX << ";" << m_minButtonY << ";" << m_maxButtonX << ";" << m_maxButtonY << ";\n";
  sessionFile << "button press per cell;\n";
  for (int y = m_minButtonY; y <= m_maxButtonY; y++) {
    for (int x = m_minButtonX; x <= m_maxButtonX; x++) {
      sessionFile << m_buttonPresses[{ x, y }] << ";";
    }
    sessionFile << "\n";
  }

  sessionFile.flush();
}

void KeyListener::onKeyPressed(const GlobalKeyEvent &ev)
{
  if (ev.keyPress != PressType_Press) return;
  if (ev.scanCode >= RECORDED_SCANCODE_COUNT) return; // very unlikely

  m_keyPresses[ev.scanCode]++;
  
  m_currentEventCount++;
  if (m_currentEventCount % SAVE_PERIOD == 0)
    dumpRecordedEvents();
}

void KeyListener::onButtonPressed(const GlobalButtonEvent &ev)
{
  if (!ev.isPressed)
    return;

  ClickPos p{ ev.cursorX / BUTTON_POSITION_THRESHOLD, ev.cursorY / BUTTON_POSITION_THRESHOLD };
  m_minButtonX = std::min(m_minButtonX, p.x);
  m_maxButtonX = std::max(m_maxButtonX, p.x);
  m_minButtonY = std::min(m_minButtonY, p.y);
  m_maxButtonY = std::max(m_maxButtonY, p.y);
  m_buttonPresses[p]++;
  
  m_currentEventCount++;
  if (m_currentEventCount % SAVE_PERIOD == 0)
    dumpRecordedEvents();
}

KeyStatsRecorder::KeyStatsRecorder()
{
  cse::keys::addGlobalKeyListener(std::make_shared<KeyListener>());
}

}

bool operator==(const ClickPos& p1, const ClickPos& p2)
{
  return p1.x == p2.x && p1.y == p2.y;
}

size_t std::hash<ClickPos>::operator()(const ClickPos& p) const noexcept
{
  // Z->N
  size_t k1 = p.x < 0 ? -2 * p.x - 1 : 2 * p.x;
  size_t k2 = p.y < 0 ? -2 * p.y - 1 : 2 * p.y;
  // Cantor paring
  return (k1 + k2) * (k1 + k2 + 1) / 2 + k2;
}
