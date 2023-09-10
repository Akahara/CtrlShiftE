#pragma once

#include "../cse.h"

namespace cse::extensions {

class UniversalShortcut : public CSEExtension {
public:
  static constexpr GlobalKeystroke KEYSTROKE{ 'E', KeyFlags_Ctrl | KeyFlags_Shift };

  UniversalShortcut();
  ~UniversalShortcut();
};

}
