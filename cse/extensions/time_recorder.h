#pragma once

#include <set>
#include <chrono>

#include "../cse.h"

class IOTTimeRecorder : public IOTGenerator {
private:
  constexpr static std::string_view COMMAND_PREFIX = "time ";
  std::set<std::string>                               m_knownRecords;
  std::vector<std::pair<std::string, std::time_t>>    m_activeRecords;
public:
  IOTTimeRecorder();
  ~IOTTimeRecorder();
  void getCommands(const std::string &text, std::vector<IOTCommand *> &out_commands) override;

  void toggleRecording(const std::string &record);
private:
  static std::string getRecordsPath();
  size_t getActiveRecordIndex(const std::string &recordName);
  void saveRecord(const std::string &recordName, std::time_t beginTime, std::time_t endTime);
};