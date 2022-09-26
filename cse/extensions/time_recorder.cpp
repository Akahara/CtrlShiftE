#include "time_recorder.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;


IOTTimeRecorder::IOTTimeRecorder()
  : m_knownRecords()
{
  for (const auto &entry : fs::directory_iterator(getRecordsPath()))
    m_knownRecords.insert(entry.path().filename().string());
}

IOTTimeRecorder::~IOTTimeRecorder()
{
  std::time_t endTime = std::time(nullptr);
  for (auto &[recordName, beginTime] : m_activeRecords)
    saveRecord(recordName, beginTime, endTime);
}

void IOTTimeRecorder::getCommands(const std::string &text, std::vector<IOTCommand*> &out_commands)
{
  if (!text.starts_with(COMMAND_PREFIX))
    return;
  std::string_view queriedRecordName{ text.begin() + COMMAND_PREFIX.length(), text.end()};

  for (const std::string &known : m_knownRecords) {
    size_t diff = cse::tools::levenshteinDistance(known, queriedRecordName);
    if (diff >= known.length() && queriedRecordName.length() > 2)
      continue;
    float relevance = 10.f * (1.f - diff/(known.length()+2.f));

    std::string suggestion = std::string(COMMAND_PREFIX) + known;

    if (getActiveRecordIndex(known) != -1) {
      suggestion += " (active)";
      relevance += 5.f;
    }

    out_commands.push_back(new NormalCommand(suggestion, relevance, [this, &known]() { toggleRecording(known); }));
  }

  if (queriedRecordName.length() > 0 && !m_knownRecords.contains(std::string(queriedRecordName))) {
    std::string queriedRecordNameCopy = std::string(queriedRecordName);
    out_commands.push_back(new NormalCommand(text + " (new)", 5.f, [this, queriedRecordNameCopy]() { toggleRecording(queriedRecordNameCopy); }));
  }
}

const char* formatDuration(std::time_t from, std::time_t to)
{
  static char formatted[50];
  double seconds = difftime(to, from);
  int secs    = static_cast<int>(seconds) % 60;
  int minutes = static_cast<int>(seconds / 60) % 60;
  int hours   = static_cast<int>(seconds / 60 / 60);
  std::stringstream ss;
  sprintf_s(formatted, "%02dh%02dm%02ds", hours, minutes, secs);
  return formatted;
}

void IOTTimeRecorder::toggleRecording(const std::string &record)
{
  size_t recordActiveIndex = getActiveRecordIndex(record);
  if (recordActiveIndex == -1) {
    cse::log("Starting record for " + record);
    m_activeRecords.push_back(std::make_pair(record, std::time(nullptr)));
    m_knownRecords.insert(record);
  } else {
    std::time_t beginTime = m_activeRecords[recordActiveIndex].second;
    std::time_t endTime = std::time(nullptr);
    cse::log("Finished record for " + record + " lasted for " + formatDuration(beginTime, endTime));
    saveRecord(record, beginTime, endTime);
    m_activeRecords.erase(m_activeRecords.begin() + recordActiveIndex);
  }
}

size_t IOTTimeRecorder::getActiveRecordIndex(const std::string &recordName)
{
  for (size_t i = 0; i < m_activeRecords.size(); i++) {
    if (m_activeRecords[i].first == recordName)
      return i;
  }
  return -1;
}

void IOTTimeRecorder::saveRecord(const std::string &recordName, std::time_t beginTime, std::time_t endTime)
{
  fs::path recordFilePath = getRecordsPath();
  recordFilePath /= recordName;
  std::ofstream recordFile{ recordFilePath, std::ios_base::app };
  std::tm beginDate;
  std::tm endDate;
  double secondsDiff = difftime(endTime, beginTime);
  localtime_s(&beginDate, &beginTime);
  localtime_s(&endDate, &endTime);
  recordFile
    << (beginDate.tm_year + 1900) << "."
    << (beginDate.tm_mon + 1) << "."
    << (beginDate.tm_mday) << "_"
    << (beginDate.tm_hour + 1) << "."
    << (beginDate.tm_min) << "."
    << (beginDate.tm_sec)
    << " to "
    << (endDate.tm_year + 1900) << "."
    << (endDate.tm_mon + 1) << "."
    << (endDate.tm_mday) << "_"
    << (endDate.tm_hour + 1) << "."
    << (endDate.tm_min) << "."
    << (endDate.tm_sec)
    << " lasting for "
    << secondsDiff
    << std::endl;
}

std::string IOTTimeRecorder::getRecordsPath()
{
  std::string recordFilesDir = cse::getUserFilesPath() + "\\time_records";
  fs::create_directories(recordFilesDir);
  return recordFilesDir;
}
