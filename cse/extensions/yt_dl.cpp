#include "yt_dl.h"

#define NOMINMAX
#include <Windows.h>

#include "cse_commands.h"

namespace cse::extensions
{

void YtDlStatusWindow::render()
{
  ImGui::RadioButton("video", &m_downloadType, YtDl::DOWNLOAD_TYPE_VIDEO); ImGui::SameLine();
  ImGui::RadioButton("music", &m_downloadType, YtDl::DOWNLOAD_TYPE_MUSIC); ImGui::SameLine();
  if (ImGui::Button("Open Download Directory"))
    cse::extensions::openFileDir(m_sharedExtension->getDownloadPath().string().c_str());
  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(222, 63, 63, 255));
  if (ImGui::Button("Cancel All"))
    m_sharedExtension->cancelAllTasks();
  ImGui::PopStyleColor();

  if(ImGui::InputText("url", m_urlBuf, sizeof(m_urlBuf), ImGuiInputTextFlags_EnterReturnsTrue) && m_urlBuf[0] != '\0') {
    m_sharedExtension->runYtDlProcess(m_urlBuf, m_downloadType);
    m_urlBuf[0] = '\0';
  }

  constexpr ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV;
  if (ImGui::BeginTable("Logs", 1, flags, { 0.f, 0.f })) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Logs", ImGuiTableColumnFlags_NoHeaderLabel);
    ImGui::TableHeadersRow();

    auto &logs = m_sharedExtension->getLogs();
    std::lock_guard _guard{ m_sharedExtension->getLogsMutex() };
    for (const std::string &line : logs) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(line.c_str());
    }

    if (m_previousLogLinesCount != logs.size()) {
      ImGui::SetScrollHereY(1.0f);
      m_previousLogLinesCount = logs.size();
    }

    ImGui::EndTable();
  }
}

YtDl::YtDl()
{
  cse::commands::addCommand({
    "ytdl",
    "download a video",
    { /* no arguments */ },
    [this](auto &args) { bringUpProgressWindow(); }
  });

  YtDl::reload();
}

void YtDl::reload()
{
  auto linkPath = cse::extensions::getUserFilesPath() / "yt-dlp.exe.lnk";
  auto normalPath = cse::extensions::getUserFilesPath() / "yt-dlp.exe";
  if(fs::is_regular_file(linkPath)) {
    m_ytdlPath = linkPath;
  } else if(fs::is_regular_file(normalPath)) {
    m_ytdlPath = normalPath;
  } else {
    cse::logm("yt-dlp not found at ", normalPath, " make sure to download yt-dlp from https://github.com/yt-dlp/yt-dlp/releases");
  }

  m_downloadPath = cse::extensions::getUserFilesPath() / "downloads";
  if (!fs::is_directory(m_downloadPath))
    fs::create_directories(m_downloadPath);
}

void YtDl::runYtDlProcess(const std::string &url, downloadtype_t downloadType)
{
  std::stringstream fullCommand;
  fullCommand
    << "cmd.exe /s /c \""
    << " \"" << m_ytdlPath.string() << "\""
    << " \"" << url << "\""
    << " -o %(uploader)s_%(title)s.%(ext)s"
    << (downloadType == DOWNLOAD_TYPE_MUSIC ? " --write-thumbnail -f bestaudio" : "")
    << "\"";
  std::string fullCommandString = fullCommand.str();
  auto fullCommandCString = std::make_unique<char[]>(fullCommandString.size() + 1);
  std::ranges::copy(fullCommandString, fullCommandCString.get());

  SECURITY_ATTRIBUTES securityAttributes;
  securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  securityAttributes.lpSecurityDescriptor = NULL;

  HANDLE pipeReadEnd = NULL, pipeWriteEnd = NULL;
  CreatePipe(&pipeReadEnd, &pipeWriteEnd, &securityAttributes, 0);
  SetHandleInformation(pipeReadEnd, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA startInf{};
  startInf.cb = sizeof(STARTUPINFO);
  startInf.hStdOutput = pipeWriteEnd;
  startInf.dwFlags |= STARTF_USESTDHANDLES;

  m_logs.emplace_back("++ " + fullCommandString);

  PROCESS_INFORMATION procInf{};
  if (!CreateProcessA(
    NULL, fullCommandCString.get(), NULL, NULL,
    TRUE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, m_downloadPath.string().c_str(),
    &startInf, &procInf))
  {
    CloseHandle(pipeWriteEnd);
    m_logs.emplace_back("!! Could not instanciate yt-dl!");
    return;
  }

  CloseHandle(pipeWriteEnd);
  m_logs.emplace_back("++ Instanciated yt-dl for " + url);

  cse::extensions::runDetached([this, procInf, pipeReadEnd] {
    {
      std::lock_guard _guard{ m_logsMutex };
      m_tasksHandles.insert(procInf.hProcess);
    }

    CHAR chBuf[4096];
    DWORD readCount;
    while (ReadFile(pipeReadEnd, chBuf, std::size(chBuf), &readCount, NULL) && readCount > 0) {
      std::lock_guard _guard{ m_logsMutex };
      m_logs.emplace_back(chBuf, readCount);
    }

    DWORD exitCode;
    if (!GetExitCodeProcess(procInf.hProcess, &exitCode)) {
      std::lock_guard _guard{ m_logsMutex };
      m_logs.emplace_back("!! Could not read the exit code of a process");
    } else {
      std::lock_guard _guard{ m_logsMutex };
      m_logs.emplace_back("++ Finished downloading");
    }
    CloseHandle(procInf.hProcess);
    CloseHandle(procInf.hThread);

    {
      std::lock_guard _guard{ m_logsMutex };
      m_tasksHandles.erase(procInf.hProcess);
    }
  });
}

void YtDl::cancelAllTasks()
{
  std::lock_guard _guard{ m_logsMutex };
  m_logs.emplace_back("!! Terminated all tasks");
  for(HANDLE hProcess : m_tasksHandles) {
    if (!TerminateProcess(hProcess, 1)) // FIX does not work, maybe the "cmd" process is terminated but not the yt-dlp one ?
      m_logs.emplace_back("!! Could not terminate a process");
  }
}

void YtDl::bringUpProgressWindow()
{
  cse::graphics::createWindow(std::make_shared<YtDlStatusWindow>(this));
}

}
