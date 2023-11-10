#include "cse_utils.h"

#include "extensions/debug_window.h"

namespace cse
{

void log(std::string_view line)
{
  cse::extensions::DebugWindowProcess::log(line);
}

}