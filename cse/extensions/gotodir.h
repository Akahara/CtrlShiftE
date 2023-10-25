#pragma once

#include "cse.h"

namespace cse::extensions {

class GotoDir : public CSEExtension {
public:
  GotoDir();
  void reload() override;
};

}
