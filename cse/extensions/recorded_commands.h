#pragma once

#include "../cse.h"

namespace cse::extensions {

class RecordedCommands : public CSEExtension {
public:
  RecordedCommands();
  ~RecordedCommands();
};

}
