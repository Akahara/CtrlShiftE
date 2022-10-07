#pragma once

namespace graphics::window_helper {

struct ImplDetails {
  bool nextImGuiViewportTransparent = false;
};

extern ImplDetails implDetails;

}