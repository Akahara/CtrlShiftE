#include "time_recorder.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <chrono>

namespace cse::extensions {

TimeRecorder::TimeRecorder()
{
  Command cmd{
    "time",
    "bring up the time recording window",
    { std::make_shared<CommandEnumPart>(std::string("action"), std::initializer_list<std::string>{ "window", "edit" }) },
    [this](auto &parts) {
      if (parts[0] == "edit")
        cse::extensions::openFileDir(getRecordsPath().string().c_str());
      else
        graphics::createWindow(std::make_shared<RecordsWindow>(this));
    }
  };
  cse::commands::addCommand(std::move(cmd));
}

TimeRecorder::~TimeRecorder()
{
  std::time_t endTime = std::time(nullptr);
  for (auto &[recordName, beginTime] : m_activeRecords)
    saveRecord(recordName, beginTime, endTime);
}

const char *TimeRecorder::formatDuration(std::time_t from, std::time_t to)
{
  static char formatted[50];
  double seconds = difftime(to, from);
  int secs = static_cast<int>(seconds) % 60;
  int minutes = static_cast<int>(seconds / 60) % 60;
  int hours = static_cast<int>(seconds / 60 / 60);
  sprintf_s(formatted, "%02dh%02dm%02ds", hours, minutes, secs);
  return formatted;
}

fs::path TimeRecorder::getRecordsPath()
{
  return cse::extensions::getUserFilesPath() / "time_records";
}

size_t TimeRecorder::getActiveRecordIndex(const std::string &recordName) const
{
  for (size_t i = 0; i < m_activeRecords.size(); i++) {
    if (m_activeRecords[i].first == recordName)
      return i;
  }
  return -1;
}

std::ofstream TimeRecorder::openRecordFile(const std::string &recordName)
{
  fs::path recordFilePath = getRecordsPath();
  recordFilePath /= recordName;
  recordFilePath += ".txt";
  std::ofstream recordFile{ recordFilePath, std::ios_base::app };
  return recordFile;
}

bool TimeRecorder::toggleRecording(const std::string &record)
{
  size_t recordActiveIndex = getActiveRecordIndex(record);
  if (recordActiveIndex == -1) {
    cse::log("Starting record for " + record);
    m_activeRecords.push_back(std::make_pair(record, std::time(nullptr)));
    openRecordFile(record); // create the file
    return true;
  } else {
    std::time_t beginTime = m_activeRecords[recordActiveIndex].second;
    std::time_t endTime = std::time(nullptr);
    cse::log("Finished record for " + record + " lasted for " + formatDuration(beginTime, endTime));
    saveRecord(record, beginTime, endTime);
    m_activeRecords.erase(m_activeRecords.begin() + recordActiveIndex);
    return false;
  }
}

void TimeRecorder::saveRecord(const std::string &recordName, std::time_t beginTime, std::time_t endTime)
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

RecordsWindow::RecordsWindow(TimeRecorder *recorder) : WindowProcess("Recordings"), m_recorder(recorder)
{
  for (const auto &entry : fs::directory_iterator(TimeRecorder::getRecordsPath())) {
    std::string record = entry.path().stem().string();
    bool isRecordActive = m_recorder->getActiveRecordIndex(record) != -1;
    m_records.push_back({ record, isRecordActive });
  }
}

bool RecordsWindow::beginWindow()
{
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5.f, 5.f });
  ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { .5f, .5f });
  bool visible = WindowProcess::beginWindow();
  ImGui::PopStyleVar(2);
  return visible;
}

void RecordsWindow::render()
{
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
  for (PlayingRecord &pr : m_records) {
    ImGui::PushStyleColor(ImGuiCol_Button, pr.active ? m_activeButtonColor : m_basicButtonColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, pr.active ? m_activeFocusedButtonColor : m_basicFocusedButtonColor);
    if (ImGui::Button(pr.record.c_str()))
      pr.active = m_recorder->toggleRecording(pr.record);
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
  }
  ImGui::PopStyleVar();
  ImGui::NewLine();
  if (ImGui::InputText("new record", m_newRecordName, sizeof(m_newRecordName), ImGuiInputTextFlags_EnterReturnsTrue)) {
    std::string record = m_newRecordName;
    auto rc = std::find_if(m_records.begin(), m_records.end(), [&](PlayingRecord &rc) { return rc.record == record; });
    PlayingRecord *updatedRecord;
    if (rc == m_records.end()) {
      m_records.push_back({ record, true });
      updatedRecord = &m_records.back();
    } else {
      updatedRecord = &*rc;
    }
    updatedRecord->active = m_recorder->toggleRecording(record);
    std::fill(std::begin(m_newRecordName), std::end(m_newRecordName), '\0');
  }
}

}
