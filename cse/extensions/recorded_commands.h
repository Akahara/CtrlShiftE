#pragma once

#include "cse.h"

namespace cse::extensions {

class RecordedCommands : public CSEExtension {
public:
  RecordedCommands();

  static void addRawCommand(const char *cseCommand, const char *tooltip, const char *systemCommand);
  static void addShellCommand(const char *cseCommand, const char *tooltip, const char *executablePath, const char *arguments);
  static void addWebCommand(const char *cseCommand, const char *tooltip, const char *url);
};

}
