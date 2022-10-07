#pragma once

#include "../cse.h"

namespace cse::extensions {

class TimeRecorder : public CSEExtension {
public:
  TimeRecorder();
  ~TimeRecorder();
};

}
