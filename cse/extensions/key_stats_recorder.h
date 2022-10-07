#pragma once

#include "../cse.h"

namespace cse::extensions {

class KeyStatsRecorder : public CSEExtension {
public:
  KeyStatsRecorder();
  ~KeyStatsRecorder();
};

}