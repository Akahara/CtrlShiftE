#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "../imgui/imgui.h"

class WindowProcess {
protected:
  std::string m_windowName;
  bool m_isVisible = true;

public:
  WindowProcess(std::string_view windowName);
  virtual ~WindowProcess() = default;

  virtual bool beginWindow();
  virtual void render() = 0;

  const std::string &getName() const { return m_windowName; }
  bool isVisible() const { return m_isVisible; }
  void setVisible(bool visible) { m_isVisible = visible; }
};

class IOTCommand {
protected:
  std::string  m_text;
  float        m_relevanceScore;
public:
  IOTCommand(const std::string &text, float relevanceScore)
    : m_text(text), m_relevanceScore(relevanceScore) { }
  virtual ~IOTCommand() = default;
  
  const std::string &getText() const { return m_text; }
  float getRelevanceScore() const { return m_relevanceScore; }

  virtual void runCommand() = 0;
};

class NormalCommand : public IOTCommand {
private:
  std::function<void()> m_callback;
public:
  NormalCommand(const std::string &text, float relevanceScore, const std::function<void()> &callback)
    : IOTCommand(text, relevanceScore), m_callback(callback) { }
  void runCommand() override { m_callback(); }
};

class IOTGenerator {
public:
  virtual ~IOTGenerator() = default;

  virtual void getCommands(const std::string &text, std::vector<IOTCommand*> &out_commands) = 0;
};

typedef unsigned char KeyFlags;

enum KeyFlags_ : KeyFlags {
  KeyFlags_None   = 0,
  KeyFlags_Option = 1 << 0,
  KeyFlags_Ctrl   = 1 << 1,
  KeyFlags_Shift  = 1 << 2,
};

struct GlobalKeyEvent {
  char keyCode;
  KeyFlags keyFlags;
  long long pressTime;
};

struct GlobalKeystroke {
  char keyCode;
  KeyFlags keyFlags;
public:
  bool match(const GlobalKeyEvent &ks) const;
};

class GlobalKeyListener {
public:
  virtual ~GlobalKeyListener() = default;

  virtual void onKeyPressed(GlobalKeyEvent ev) = 0;
};

namespace cse {

void log(std::string_view line);
void logErr(std::string_view line);
void logInfo(std::string_view line);

void addIOTGenerator(const std::shared_ptr<IOTGenerator> &generator);

const std::string &getUserFilesPath();

namespace window_helper {

void prepareAlwaysOnTop();

}

namespace keys {

void addGlobalySuppressedKeystroke(GlobalKeystroke keystroke);
void addGlobalKeyListener(const std::shared_ptr<GlobalKeyListener> &listener);

}

namespace tools {

size_t levenshteinDistance(std::string_view source, std::string_view target);

}

}
