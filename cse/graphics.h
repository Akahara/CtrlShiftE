#pragma once

#include <string>
#include <memory>

#include "../imgui/imgui.h"

#include "cse.h"

namespace graphics {

void loadGraphics();
void destroyGraphics();

void render();
bool shouldDispose();

void createWindow(const std::shared_ptr<WindowProcess> &process);
void closeAllWindows();

}