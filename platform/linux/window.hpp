#pragma once

#include "input/keyboard.hpp"
#include "input/mouse.hpp"

#include <functional>
#include <string>

// Forward declaration
struct GLFWwindow;

namespace tide::platform::linux {

/**
 * GLFW window wrapper for the terminal.
 * Handles window creation, OpenGL context, and event loop.
 */
class Window {
public:
  using ResizeCallback = std::function<void(int width, int height)>;
  using CloseCallback = std::function<void()>;

  Window();
  ~Window();

  // Non-copyable
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  /**
   * Create the window with specified dimensions.
   * @param width Window width in pixels
   * @param height Window height in pixels
   * @param title Window title
   * @return true if window created successfully
   */
  bool create(int width, int height, const std::string &title);

  /**
   * Destroy the window and clean up resources.
   */
  void destroy();

  /**
   * Process pending window events.
   * Should be called once per frame.
   */
  void poll_events();

  /**
   * Swap front and back buffers.
   */
  void swap_buffers();

  /**
   * Check if the window should close.
   */
  [[nodiscard]] bool should_close() const;

  /**
   * Request the window to close.
   */
  void request_close();

  /**
   * Get current window dimensions.
   */
  [[nodiscard]] int width() const { return width_; }
  [[nodiscard]] int height() const { return height_; }

  /**
   * Get the GLFW window handle.
   */
  [[nodiscard]] GLFWwindow *handle() const { return window_; }

  /**
   * Set resize callback.
   */
  void set_resize_callback(ResizeCallback callback) {
    resize_callback_ = std::move(callback);
  }

  /**
   * Set close callback.
   */
  void set_close_callback(CloseCallback callback) {
    close_callback_ = std::move(callback);
  }

  /**
   * Get the keyboard input handler.
   */
  input::Keyboard &keyboard() { return keyboard_; }

  /**
   * Get the mouse input handler.
   */
  input::Mouse &mouse() { return mouse_; }

private:
  GLFWwindow *window_ = nullptr;
  int width_ = 0;
  int height_ = 0;

  input::Keyboard keyboard_;
  input::Mouse mouse_;

  ResizeCallback resize_callback_;
  CloseCallback close_callback_;

  // Static callbacks for GLFW (bridge to instance methods)
  static void framebuffer_size_callback(GLFWwindow *window, int width,
                                        int height);
  static void key_callback(GLFWwindow *window, int key, int scancode,
                           int action, int mods);
  static void char_callback(GLFWwindow *window, unsigned int codepoint);
  static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                    int mods);
  static void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos);
  static void scroll_callback(GLFWwindow *window, double xoffset,
                              double yoffset);
};

} // namespace tide::platform::linux
