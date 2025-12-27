#pragma once

#include <functional>

namespace tide::input {

/**
 * Mouse input handler.
 *
 * TODO: Implement mouse handling:
 * - Button press/release events
 * - Cursor position tracking
 * - Scroll wheel events
 * - Text selection
 * - Drag-and-drop support
 */
class Mouse {
public:
  using ButtonCallback = std::function<void(int button, int action, int mods)>;
  using MoveCallback = std::function<void(double x, double y)>;
  using ScrollCallback = std::function<void(double xoffset, double yoffset)>;

  Mouse() = default;

  /**
   * Set the callback for button press/release events.
   */
  void set_button_callback(ButtonCallback callback) {
    button_callback_ = std::move(callback);
  }

  /**
   * Set the callback for cursor movement.
   */
  void set_move_callback(MoveCallback callback) {
    move_callback_ = std::move(callback);
  }

  /**
   * Set the callback for scroll wheel events.
   */
  void set_scroll_callback(ScrollCallback callback) {
    scroll_callback_ = std::move(callback);
  }

  /**
   * Process a button event from GLFW.
   */
  void on_button(int button, int action, int mods);

  /**
   * Process a cursor movement event from GLFW.
   */
  void on_move(double x, double y);

  /**
   * Process a scroll event from GLFW.
   */
  void on_scroll(double xoffset, double yoffset);

  // Getters for current state
  [[nodiscard]] double cursor_x() const { return cursor_x_; }
  [[nodiscard]] double cursor_y() const { return cursor_y_; }

private:
  ButtonCallback button_callback_;
  MoveCallback move_callback_;
  ScrollCallback scroll_callback_;

  double cursor_x_ = 0.0;
  double cursor_y_ = 0.0;

  // TODO: Add selection state
  // bool selecting_ = false;
  // int selection_start_col_, selection_start_row_;
  // int selection_end_col_, selection_end_row_;
};

} // namespace tide::input
