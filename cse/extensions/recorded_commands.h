#pragma once

#include "../cse.h"

class IOTRecordedCommands : public IOTGenerator {
public:
  void getCommands(const std::string &text, std::vector<IOTCommand *> &out_commands) override;
};