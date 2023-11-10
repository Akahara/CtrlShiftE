#include "debug_window.h"

#include <vector>
#include <string>

namespace cse::extensions
{

std::vector<std::string> DebugWindowProcess::s_logLines;

DebugWindowProcess::DebugWindowProcess()
  : WindowProcess("CtrlShiftE.Logs")
{
}

void DebugWindowProcess::render()
{
  ImGui::Checkbox("Autoscroll", &m_autoScroll);

  constexpr ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV;

  ImVec2 outer_size = ImVec2(0.0f, 0.f);
  if (ImGui::BeginTable("Logs", 1, flags, outer_size)) {

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Logs", ImGuiTableColumnFlags_NoHeaderLabel);
    ImGui::TableHeadersRow();

    for (std::string &line : s_logLines) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(line.c_str());
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
      ImGui::SetScrollHereY(1.0f);
    ImGui::EndTable();
  }
}

void DebugWindowProcess::log(std::string_view line)
{
  if (s_logLines.size() > 1000) s_logLines.resize(500);
  s_logLines.emplace_back(line);
}

}