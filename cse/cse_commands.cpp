#define CSE_EXPOSE_INTERNALS
#include "cse_commands.h"
#undef CSE_EXPOSE_INTERNALS

static bool cstrStartsWith(const char *str, std::string_view part)
{
  for (size_t i = 0; i < part.length(); i++) {
    if (str[i] == '\0' || str[i] != part[i])
      return false;
  }
  return true;
}

CommandEnumPart::CommandEnumPart(std::string placeHolder, std::initializer_list<const char *> parts)
  : CommandPart(std::move(placeHolder))
{
  std::ranges::copy(parts, std::back_inserter(m_parts));
}

CommandEnumPart::CommandEnumPart(std::string placeHolder, std::vector<std::string> parts)
  : CommandPart(std::move(placeHolder)), m_parts(std::move(parts))
{
}

void CommandEnumPart::getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const
{
  for (const auto& m_part : m_parts)
  {
    if (cstrStartsWith(m_part.c_str(), part))
      out_completions.push_back({ m_part.c_str() + part.length() });
  }
}

bool CommandEnumPart::isGood(std::string_view part) const
{
  return std::ranges::find(m_parts, part) != m_parts.end();
}

CommandSaveFilePart::CommandSaveFilePart(const std::string &placeHolder, const fs::path &dirPath, bool allowCreate)
  : CommandPart(placeHolder)
  , m_dirPath(dirPath)
  , m_allowCreate(allowCreate)
{
  if (!fs::exists(dirPath))
    fs::create_directories(dirPath);
}

void CommandSaveFilePart::getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const
{
  bool fileExists = false;
  for (auto &available : m_availableFiles) {
    if (std::string availableStr = available.string(); availableStr.starts_with(part)) {
      float score = static_cast<float>(part.size());
      out_completions.push_back({ availableStr.substr(part.size()), score });
      if (availableStr.size() == part.size())
        fileExists = true;
    }
  }
  if (m_allowCreate && !fileExists && !part.empty() && isInFilenameCharset(part)) {
    out_completions.push_back({ "", .5f, "new" });
  }
}

bool CommandSaveFilePart::isGood(std::string_view part) const
{
  if (part.empty())
    return false;
  if (m_allowCreate && isInFilenameCharset(part))
    return true;
  if (std::ranges::any_of(m_availableFiles, [&part](auto &available) { return available.string() == part; }))
    return true;
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
  constexpr char specialCharacters[] = "_-+=.";
  return std::ranges::all_of(text, [](char c) {
    return
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      std::ranges::find(specialCharacters, c) != std::end(specialCharacters);
  });
}


namespace cse::commands
{

static std::vector<Command> s_commands;

void addCommand(Command &&command)
{
  logm("Added command ", command.prefix);
  s_commands.push_back(std::move(command));
}

bool hasCommand(const std::string &prefix)
{
  return std::ranges::any_of(s_commands, [&](const Command &cmd) { return cmd.prefix == prefix; });
}

void removeCommand(const std::string &prefix)
{
  if (std::erase_if(s_commands, [&](const Command &command) { return command.prefix == prefix; }))
    logm("Removed command ", prefix);
}

bool executeCommand(const std::string &cmd)
{
  size_t split = 0;
  std::string_view part;
  auto nextPart = [&]
  {
    size_t previousSplit = split;
    if (split >= cmd.length()) return false;
    split = cmd.find(' ', split);
    if (split == std::string::npos) split = cmd.length();
    part = std::string_view(cmd.data() + previousSplit, split - previousSplit);
    split++;
    return true;
  };

  if (!nextPart()) return false;
  auto commandIt = std::ranges::find_if(s_commands, [&](const Command &command) { return command.prefix == part; });
  if (commandIt == s_commands.end()) return false;
  Command &command = *commandIt;
  std::vector<std::string_view> arguments;
  for(auto &cmdpart : command.parts)
  {
    if (!nextPart()) return false;
    if (!cmdpart->isGood(part)) return false;
    arguments.push_back(part);
  }
  if (nextPart()) return false;

  command.executor(arguments);
  return true;
}

std::vector<Command> &getCommands()
{
  return s_commands;
}

}