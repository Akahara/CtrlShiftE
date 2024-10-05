#include "time_recorder.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <chrono>

using namespace std::chrono_literals;

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
  for (auto &[recordName, beginTime] : m_activeRecords)
    saveRecord(recordName, beginTime, clock::now());
}

const char *TimeRecorder::formatDuration(const time_point &from, const time_point &to)
{
  static char formatted[50];
  int seconds = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(to - from).count());
  int secs    = seconds % 60;
  int minutes = seconds / 60 % 60;
  int hours   = seconds / 60 / 60;
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
  if (!fs::is_directory(recordFilePath))
    fs::create_directories(recordFilePath);
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
    m_activeRecords.emplace_back(record, clock::now());
    openRecordFile(record); // create the file
    return true;
  } else {
    const time_point &beginTime = m_activeRecords[recordActiveIndex].second;
    time_point endTime = clock::now();
    cse::log("Finished record for " + record + " lasted for " + formatDuration(beginTime, endTime));
    saveRecord(record, beginTime, endTime);
    m_activeRecords.erase(m_activeRecords.begin() + recordActiveIndex);
    return false;
  }
}

void TimeRecorder::saveRecord(const std::string &recordName, const time_point &beginTime, const time_point &endTime)
{
  std::ofstream recordFile = openRecordFile(recordName);
  auto now = std::chrono::system_clock::now();
  time_t beginTimeT = clock::to_time_t(beginTime);
  time_t endTimeT = clock::to_time_t(endTime);
  std::tm beginDate;
  std::tm endDate;
  localtime_s(&beginDate, &beginTimeT);
  localtime_s(&endDate, &endTimeT);
  double secondsDiff = difftime(endTimeT, beginTimeT);
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

RecordsWindow::RecordsWindow(TimeRecorder *recorder)
  : WindowProcess("Recordings")
  , m_recorder(recorder)
{
  fs::path recordsPath = TimeRecorder::getRecordsPath();
  if (!fs::is_directory(recordsPath)) {
    fs::create_directories(recordsPath);
    return;
  }
  for (const auto &entry : fs::directory_iterator(recordsPath)) {
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
  bool isAnyPlaying = std::ranges::any_of(m_records, [](auto r) { return r.active; });

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
    auto rc = std::ranges::find_if(m_records, [&](const PlayingRecord &rc) { return rc.record == record; });
    PlayingRecord *updatedRecord;
    if (rc == m_records.end()) {
      m_records.push_back({ record, true });
      updatedRecord = &m_records.back();
    } else {
      updatedRecord = &*rc;
    }
    updatedRecord->active = m_recorder->toggleRecording(record);
    std::ranges::fill(m_newRecordName, '\0');
  }
  if (!isAnyPlaying) ImGui::BeginDisabled();
  ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(117, 184, 83, 255));
  if (ImGui::Button("+1h"   )) addFixedTimeToActiveRecords(1h);    ImGui::SameLine(0,4);
  if (ImGui::Button("+30min")) addFixedTimeToActiveRecords(30min); ImGui::SameLine(0,4);
  if (ImGui::Button("+15min")) addFixedTimeToActiveRecords(15min); ImGui::SameLine(0,4);
  ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(184, 103, 83, 255));
  if (ImGui::Button("-30min")) addFixedTimeToActiveRecords(-30min); ImGui::SameLine(0,4);
  if (ImGui::Button("-15min")) addFixedTimeToActiveRecords(-15min); ImGui::SameLine(0,4);
  ImGui::PopStyleColor(2);
  if (!isAnyPlaying) ImGui::EndDisabled();
}

void RecordsWindow::addFixedTimeToActiveRecords(const TimeRecorder::clock::duration &fixedDuration) const
{
  auto current = TimeRecorder::clock::now();
  for (const auto &[record, active] : m_records) {
    if (active)
      m_recorder->saveRecord(record, current - fixedDuration, current);
  }
}
}
