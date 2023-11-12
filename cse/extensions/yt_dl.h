#pragma once

#include <unordered_set>

#include "cse_extensions.h"
#include "cse_graphics.h"

namespace cse::extensions {

class YtDl : public CSEExtension {
public:
  static constexpr const char *EXTENSION_NAME = "YtDl";

  using downloadtype_t = int;
  static constexpr downloadtype_t DOWNLOAD_TYPE_VIDEO = 0, DOWNLOAD_TYPE_MUSIC = 1;

  YtDl();

  void reload() override;

  const std::vector<std::string> &getLogs() const { return m_logs; }
  std::mutex &getLogsMutex() { return m_logsMutex; }
  const fs::path &getDownloadPath() const { return m_downloadPath; }

  void runYtDlProcess(const std::string &url, downloadtype_t downloadType);
  void cancelAllTasks();

private:
  void bringUpProgressWindow();

  fs::path m_ytdlPath;
  fs::path m_downloadPath;

  std::vector<std::string> m_logs;
  std::mutex m_logsMutex;
  std::unordered_set<void *> m_tasksHandles;
};

class YtDlStatusWindow : public WindowProcess {
public:
  explicit YtDlStatusWindow(YtDl *sharedExtension)
    : WindowProcess("yt-dl")
    , m_sharedExtension(sharedExtension)
  {}

  void render() override;

private:

  YtDl *m_sharedExtension;
  //char m_urlBuf[100] = "\0";
  char m_urlBuf[100] = "https://www.youtube.com/watch?v=qkLOhd6iwVA";
  YtDl::downloadtype_t m_downloadType = YtDl::DOWNLOAD_TYPE_VIDEO;
  size_t m_previousLogLinesCount = 0;
};

}
