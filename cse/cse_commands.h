#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>

#include "cse_utils.h"

struct CommandCompletion {
  std::string missingPart;
  float relevanceScore = 1.f;
  std::string info;
};

class CommandPart {
public:
  CommandPart(std::string placeHolder)
    : m_placeHolder{std::move(placeHolder)} {}
  virtual ~CommandPart() = default;

  DELETE_COPY_OPERATORS(CommandPart)

  const std::string &getPlaceHolder() const { return m_placeHolder; }
  virtual void getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const = 0;
  virtual bool isGood(std::string_view part) const = 0;
  virtual void updateState() {}

private:
  std::string m_placeHolder;
};

class CommandEnumPart : public CommandPart {
public:
  CommandEnumPart(std::string placeHolder, std::initializer_list<const char *> parts);
  CommandEnumPart(std::string placeHolder, std::vector<std::string> parts);

  DELETE_COPY_OPERATORS(CommandEnumPart)

  void getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const override;
  bool isGood(std::string_view part) const override;
  void setParts(const std::vector<std::string> &parts) { m_parts = parts; /* note the copy */ }

private:
  std::vector<std::string> m_parts;
};

class CommandTextPart : public CommandPart {
public:
  explicit CommandTextPart(std::string placeHolder) : CommandPart(std::move(placeHolder)) {}

  void getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const override {}
  bool isGood(std::string_view part) const override { return true; }
};

class CommandSaveFilePart : public CommandPart {
public:
  explicit CommandSaveFilePart(const std::string &placeHolder, const std::filesystem::path &dirPath, bool allowCreate = true);

  void getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const override;
  bool isGood(std::string_view part) const override;

  void updateState() override;

  static bool isInFilenameCharset(std::string_view text);

protected:
  std::filesystem::path              m_dirPath;
  bool                               m_allowCreate;
  std::vector<std::filesystem::path> m_availableFiles;
};

struct Command {
  std::string prefix;
  std::string tooltip;
  std::vector<std::shared_ptr<CommandPart>> parts;
  Executor    executor;
};

namespace cse::commands
{

void addCommand(Command &&command);

#ifdef CSE_EXPOSE_INTERNALS
std::vector<Command> &getCommands();
#endif

}