#pragma once

#include <functional>

// Forward declaration for GLFW
struct GLFWwindow;

namespace tide::input {

/**
 * Keyboard input handler.
 *
 * TODO: Implement keyboard handling:
 * - Key press/release events
 * - Character input for text
 * - Modifier key tracking (Ctrl, Alt, Shift, Super)
 * - Key repeat handling
 * - Keybinding system
 */
class Keyboard {
public:
  using KeyCallback =
      std::function<void(int key, int scancode, int action, int mods)>;
  using CharCallback = std::function<void(unsigned int codepoint)>;

  Keyboard() = default;

  /**
   * Set the callback for key press/release events.
   */
  void set_key_callback(KeyCallback callback) {
    key_callback_ = std::move(callback);
  }

  /**
   * Set the callback for character input.
   */
  void set_char_callback(CharCallback callback) {
    char_callback_ = std::move(callback);
  }

  /**
   * Process a key event from GLFW.
   * Called by the window's GLFW key callback.
   */
  void on_key(int key, int scancode, int action, int mods);

  /**
   * Process a character input event from GLFW.
   * Called by the window's GLFW char callback.
   */
  void on_char(unsigned int codepoint);

  // TODO: Add methods for checking key states
  // bool is_key_pressed(int key) const;
  // bool is_key_held(int key) const;
  // bool is_modifier_active(int modifier) const;

private:
  KeyCallback key_callback_;
  CharCallback char_callback_;

  // TODO: Add key state tracking
  // std::unordered_set<int> pressed_keys_;
  // int modifier_state_ = 0;
};

} // namespace tide::input
