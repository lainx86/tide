#include "input/keyboard.hpp"

#include <GLFW/glfw3.h>
#include <iostream>


namespace tide::input {

void Keyboard::on_key(int key, int scancode, int action, int mods) {
  // TODO: Track key states
  // TODO: Implement keybindings

  // Log for debugging
  if (action == GLFW_PRESS) {
    std::cout << "[tide::Keyboard] Key pressed: " << key << " (mods: " << mods
              << ")" << std::endl;
  }

  if (key_callback_) {
    key_callback_(key, scancode, action, mods);
  }
}

void Keyboard::on_char(unsigned int codepoint) {
  // Log for debugging
  std::cout << "[tide::Keyboard] Char input: " << static_cast<char>(codepoint)
            << " (U+" << std::hex << codepoint << std::dec << ")" << std::endl;

  if (char_callback_) {
    char_callback_(codepoint);
  }
}

} // namespace tide::input
