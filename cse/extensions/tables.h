#pragma once

#include <map>

#include "../cse.h"
#include "../graphics.h"

namespace cse::extensions {

typedef std::function<WindowProcess *()> WindowProvider;

class Tables : public CSEExtension {
private:
  std::map<const char*, WindowProvider> m_tables;
public:
  Tables();
  ~Tables();
};

}
