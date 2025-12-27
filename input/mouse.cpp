#include "input/mouse.hpp"

#include <iostream>

namespace tide::input {

void Mouse::on_button(int button, int action, int mods) {
  // TODO: Handle text selection
  // TODO: Handle copy/paste via middle-click

  std::cout << "[tide::Mouse] Button " << button << " "
            << (action ? "pressed" : "released") << " at (" << cursor_x_ << ", "
            << cursor_y_ << ")"
            << " (mods: " << mods << ")" << std::endl;

  if (button_callback_) {
    button_callback_(button, action, mods);
  }
}

void Mouse::on_move(double x, double y) {
  cursor_x_ = x;
  cursor_y_ = y;

  // TODO: Update selection if dragging

  if (move_callback_) {
    move_callback_(x, y);
  }
}

void Mouse::on_scroll(double xoffset, double yoffset) {
  // TODO: Implement scrollback buffer navigation

  std::cout << "[tide::Mouse] Scroll: (" << xoffset << ", " << yoffset << ")"
            << std::endl;

  if (scroll_callback_) {
    scroll_callback_(xoffset, yoffset);
  }
}

} // namespace tide::input
