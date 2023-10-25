#pragma once

#include "cse.h"

namespace cse::extensions {

namespace fs = std::filesystem;

class TimeRecorder : public CSEExtension {
public:
  TimeRecorder();
  ~TimeRecorder() override;

  static fs::path getRecordsPath();
  static const char *formatDuration(std::time_t from, std::time_t to);
  static void saveRecord(const std::string &recordName, std::time_t beginTime, std::time_t endTime);
  static std::ofstream openRecordFile(const std::string &recordName);
  bool toggleRecording(const std::string &record);
  size_t getActiveRecordIndex(const std::string &recordName) const;

private:
  std::vector<std::pair<std::string, std::time_t>> m_activeRecords;
};

class RecordsWindow : public WindowProcess {
public:
  RecordsWindow(TimeRecorder *recorder);

private:
  struct PlayingRecord {
    std::string record;
    bool active;
  };

  bool beginWindow() override;
  void render() override;

  const ImU32 m_basicButtonColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Button]);
  const ImU32 m_basicFocusedButtonColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
  const ImU32 m_activeButtonColor = IM_COL32(15, 128, 15, 255);
  const ImU32 m_activeFocusedButtonColor = IM_COL32(50, 178, 50, 255);
  char        m_newRecordName[32]{};
  std::vector<PlayingRecord> m_records;
  TimeRecorder              *m_recorder;
};

}
