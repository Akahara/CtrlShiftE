#pragma once

#include <vector>
#include <memory>

#include "cse.h"

namespace cse {

const std::vector<std::shared_ptr<IOTGenerator>> &getIOTGenerators();
void clearIOTGenerators();


namespace keys {

void registerGlobalHook();
void unregisterGlobalHook();

// called by the main thread to process global events
void pollEvents();

}

}