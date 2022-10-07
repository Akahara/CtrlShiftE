#include "commands.h"

#include <filesystem>

namespace fs = std::filesystem;

static bool cstrStartsWith(const char *str, std::string_view part)
{
  for (size_t i = 0; i < part.length(); i++) {
    if (str[i] == '\0' || str[i] != part[i])
      return false;
  }
  return true;
}

CommandEnumPart::CommandEnumPart(const std::string &placeHolder, std::initializer_list<const char *> parts)
  : CommandPart(placeHolder)
{
  m_partsCount = parts.size();
  m_parts = new const char*[m_partsCount];
  auto iterator = parts.begin();
  for (size_t i = 0; i < m_partsCount; i++) {
    m_parts[i] = *iterator;
    iterator++;
  }
}

CommandEnumPart::CommandEnumPart(const std::string &placeHolder, const std::vector<const char *> &parts)
  : CommandPart(placeHolder)
{
  m_partsCount = parts.size();
  m_parts = new const char *[m_partsCount];
  auto iterator = parts.begin();
  for (size_t i = 0; i < m_partsCount; i++) {
    m_parts[i] = *iterator;
    iterator++;
  }
}

CommandEnumPart::~CommandEnumPart()
{
  delete[] m_parts;
}

void CommandEnumPart::getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const
{
  for (size_t i = 0; i < m_partsCount; i++) {
    if (cstrStartsWith(m_parts[i], part))
      out_completions.push_back({ m_parts[i] + part.length() });
  }
}

bool CommandEnumPart::isGood(std::string_view part) const
{
  for (size_t i = 0; i < m_partsCount; i++) {
    if (part == m_parts[i])
      return true;
  }
  return false;
}

CommandSaveFilePart::CommandSaveFilePart(const std::string &placeHolder, const fs::path dirPath, bool allowCreate)
  : CommandPart(placeHolder),
  m_dirPath(dirPath),
  m_allowCreate(allowCreate),
  m_availableFiles()
{
  if (!fs::exists(dirPath))
    fs::create_directories(dirPath);
}

void CommandSaveFilePart::getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const
{
  bool fileExists = false;
  for (auto &available : m_availableFiles) {
    std::string availableStr = available.string();
    if (availableStr.starts_with(part)) {
      float score = (float)part.size();
      out_completions.push_back({ availableStr.substr(part.size()), score });
      if (availableStr.size() == part.size())
        fileExists = true;
    }
  }
  if (m_allowCreate && !fileExists && part.size() > 0 && isInFilenameCharset(part)) {
    out_completions.push_back({ "", .5f, "new" });
  }
}

bool CommandSaveFilePart::isGood(std::string_view part) const
{
  if (part.size() == 0)
    return false;
  if (m_allowCreate && isInFilenameCharset(part))
    return true;
  for (auto &available : m_availableFiles) {
    if (available.string() == part)
      return true;
  }
  return false;
}

void CommandSaveFilePart::updateState()
{
  m_availableFiles.clear();
  for (const auto &entry : fs::directory_iterator(m_dirPath))
    m_availableFiles.push_back(entry.path().stem());
}

bool CommandSaveFilePart::isInFilenameCharset(std::string_view text)
{
  static std::string chars = "_-+=.";
  for (char c : text) {
    if (!(
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      chars.find(c) != -1))
      return false;
  }
  return true;
}
