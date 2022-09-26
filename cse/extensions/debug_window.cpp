#include "debug_window.h"

#include <vector>
#include <string>
#include <iostream>

static std::vector<const char*> logLines;

DebugWindowProcess::DebugWindowProcess()
  : WindowProcess("Logs")
{
}

bool DebugWindowProcess::beginWindow()
{
  cse::window_helper::prepareAlwaysOnTop();
  return WindowProcess::beginWindow();
}

void DebugWindowProcess::render()
{
  ImGui::Checkbox("Autoscroll", &m_autoScroll);

  constexpr ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV;

  ImVec2 outer_size = ImVec2(0.0f, 0.f);
  if (ImGui::BeginTable(tableName, 1, flags, outer_size)) {

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn(tableName, ImGuiTableColumnFlags_NoHeaderLabel);
    ImGui::TableHeadersRow();

    for (const char *line : logLines) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::Text(line);
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
      ImGui::SetScrollHereY(1.0f);
    ImGui::EndTable();
  }
}

void DebugWindowProcess::log(std::string_view line)
{
  char *copy = new char[line.length()+1];
  line.copy(copy, line.length());
  copy[line.length()] = '\0';
  logLines.push_back(copy);
  // TODO clear logLines somewhere/somewhen
}
