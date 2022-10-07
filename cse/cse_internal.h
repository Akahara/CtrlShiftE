#pragma once

#include <vector>
#include <memory>

#include "cse.h"

namespace cse {

std::vector<Command> &getCommands();


namespace keys {

void registerGlobalHook();
void unregisterGlobalHook();

// called by the main thread to process global events
void pollEvents();

}

}