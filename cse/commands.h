#pragma once

#include <string>
#include <vector>
#include <functional>

class CommandPart {
public:
  virtual void getCompletions(std::string_view part, std::vector<std::string> &out_completions) = 0;
  virtual bool isGood(std::string_view part) = 0;
};

class NormalPart : public CommandPart {
private:
  const char **m_parts;
  size_t m_partsCount;
public:
  NormalPart(const char **parts, size_t partCount)
    : m_parts(parts), m_partsCount(partCount) { }

  static bool cstrStartsWith(const char *str, std::string_view part)
  {
    for (size_t i = 0; i < part.length(); i++) {
      if (str[i] == '\0' || str[i] != part[i])
        return false;
    }
    return true;
  }

  void getCompletions(std::string_view part, std::vector<std::string> &out_completions) override
  {
    for (size_t i = 0; i < m_partsCount; i++) {
      if(cstrStartsWith(m_parts[i], part))
        out_completions.push_back(m_parts[i] + part.length());
    }
  }

  bool isGood(std::string_view part) override
  {
    for (size_t i = 0; i < m_partsCount; i++) {
      if (part == m_parts[i])
        return true;
    }
    return false;
  }
};

struct Command {
  CommandPart *parts[16];
  size_t length;
  std::function<void(const std::vector<std::string_view> &parts)> executor;
};
