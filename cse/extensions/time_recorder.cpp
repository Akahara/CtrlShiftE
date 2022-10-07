#include "time_recorder.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;


static struct TimeRecorderData {
  std::vector<std::pair<std::string, std::time_t>> activeRecords;
} *data;


static const char* formatDuration(std::time_t from, std::time_t to)
{
  static char formatted[50];
  double seconds = difftime(to, from);
  int secs    = static_cast<int>(seconds) % 60;
  int minutes = static_cast<int>(seconds / 60) % 60;
  int hours   = static_cast<int>(seconds / 60 / 60);
  sprintf_s(formatted, "%02dh%02dm%02ds", hours, minutes, secs);
  return formatted;
}

static fs::path getRecordsPath()
{
  return cse::getUserFilesPath() / "time_records";
}

static fs::path getRecordsPath();
static void toggleRecording(const std::string &record);
static void saveRecord(const std::string &recordName, std::time_t beginTime, std::time_t endTime);

static size_t getActiveRecordIndex(const std::string &recordName)
{
  for (size_t i = 0; i < data->activeRecords.size(); i++) {
    if (data->activeRecords[i].first == recordName)
      return i;
  }
  return -1;
}

static std::ofstream openRecordFile(const std::string &recordName)
{
  fs::path recordFilePath = getRecordsPath();
  recordFilePath /= recordName;
  recordFilePath += ".txt";
  std::ofstream recordFile{ recordFilePath, std::ios_base::app };
  return recordFile;
}

static void toggleRecording(const std::string &record)
{
  size_t recordActiveIndex = getActiveRecordIndex(record);
  if (recordActiveIndex == -1) {
    cse::log("Starting record for " + record);
    data->activeRecords.push_back(std::make_pair(record, std::time(nullptr)));
    openRecordFile(record); // create the file
  } else {
    std::time_t beginTime = data->activeRecords[recordActiveIndex].second;
    std::time_t endTime = std::time(nullptr);
    cse::log("Finished record for " + record + " lasted for " + formatDuration(beginTime, endTime));
    saveRecord(record, beginTime, endTime);
    data->activeRecords.erase(data->activeRecords.begin() + recordActiveIndex);
  }
}

static void saveRecord(const std::string &recordName, std::time_t beginTime, std::time_t endTime)
{
  std::ofstream recordFile = openRecordFile(recordName);
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

class TimeFileCommandPart : public CommandSaveFilePart {
public:
  TimeFileCommandPart()
    : CommandSaveFilePart("name", getRecordsPath(), true)
  {
  }

  void getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const override
  {
    bool fileExists = false;
    for (auto &available : m_availableFiles) {
      std::string availableStr = available.string();
      if (availableStr.starts_with(part)) {
        bool isRunningTimer = getActiveRecordIndex(availableStr) != -1;
        float score = (float)part.size() + (isRunningTimer * 5.f);
        out_completions.push_back({ availableStr.substr(part.size()), score, isRunningTimer ? "active":"" });
        if (availableStr.size() == part.size())
          fileExists = true;
      }
    }
    if (m_allowCreate && !fileExists && part.size() > 0 && isInFilenameCharset(part)) {
      out_completions.push_back({ "", .5f, "new" });
    }
  }
};


namespace cse::extensions {

TimeRecorder::TimeRecorder()
{
  Command cmd{
    "time",
    "start/stop a timer",
    { new TimeFileCommandPart },
    [](auto &parts) { toggleRecording(std::string(parts[0])); }
  };
  cse::addCommand(std::move(cmd));
  
  data = new TimeRecorderData;
}

TimeRecorder::~TimeRecorder()
{
  std::time_t endTime = std::time(nullptr);
  for (auto &[recordName, beginTime] : data->activeRecords)
    saveRecord(recordName, beginTime, endTime);
  delete data;
}

}