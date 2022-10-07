#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>

struct CommandCompletion {
  std::string missingPart;
  float relevanceScore = 1.f;
  std::string info;
};

class CommandPart {
private:
  std::string m_placeHolder;
public:
  CommandPart(const std::string &placeHolder)
    : m_placeHolder{ placeHolder } { }

  const std::string &getPlaceHolder() const { return m_placeHolder; }
  virtual void getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const = 0;
  virtual bool isGood(std::string_view part) const = 0;
  virtual void updateState() {}
};

class CommandEnumPart : public CommandPart {
private:
  const char **m_parts;
  size_t       m_partsCount;
public:
  CommandEnumPart(const std::string &placeHolder, std::initializer_list<const char *> parts);
  CommandEnumPart(const std::string &placeHolder, const std::vector<const char *> &parts);
  ~CommandEnumPart();

  void getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const override;
  bool isGood(std::string_view part) const override;
};

class CommandTextPart : public CommandPart {
public:
  CommandTextPart(const std::string &placeHolder) : CommandPart(placeHolder) {}

  void getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const override {}
  bool isGood(std::string_view part) const override { return true; }
};

class CommandSaveFilePart : public CommandPart {
protected:
  std::filesystem::path              m_dirPath;
  bool                               m_allowCreate;
  std::vector<std::filesystem::path> m_availableFiles;
public:
  CommandSaveFilePart(const std::string &placeHolder, const std::filesystem::path dirPath, bool allowCreate=true);

  void getCompletions(std::string_view part, std::vector<CommandCompletion> &out_completions) const override;
  bool isGood(std::string_view part) const override;

  void updateState() override;

  static bool isInFilenameCharset(std::string_view text);
};

//typedef void (*Executor)(const std::vector<std::string_view> &parts);
typedef std::function<void(const std::vector<std::string_view> &parts)> Executor;

struct Command {
  std::string               prefix;
  std::string               tooltip;
  std::vector<CommandPart*> parts;
  Executor                  executor;
};
