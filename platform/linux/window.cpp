#include "platform/linux/window.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>


#include <iostream>

namespace tide::platform::linux {

Window::Window() = default;

Window::~Window() { destroy(); }

bool Window::create(int width, int height, const std::string &title) {
  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "[tide::Window] Failed to initialize GLFW" << std::endl;
    return false;
  }

  // Configure OpenGL context
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // Create window
  window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
  if (!window_) {
    std::cerr << "[tide::Window] Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return false;
  }

  // Make OpenGL context current
  glfwMakeContextCurrent(window_);

  // Load OpenGL functions via GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "[tide::Window] Failed to initialize GLAD" << std::endl;
    glfwDestroyWindow(window_);
    glfwTerminate();
    window_ = nullptr;
    return false;
  }

  // Store dimensions
  glfwGetFramebufferSize(window_, &width_, &height_);

  // Store this pointer for callbacks
  glfwSetWindowUserPointer(window_, this);

  // Set up callbacks
  glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);
  glfwSetKeyCallback(window_, key_callback);
  glfwSetCharCallback(window_, char_callback);
  glfwSetMouseButtonCallback(window_, mouse_button_callback);
  glfwSetCursorPosCallback(window_, cursor_pos_callback);
  glfwSetScrollCallback(window_, scroll_callback);

  // Enable vsync
  glfwSwapInterval(1);

  std::cout << "[tide::Window] Created " << width_ << "x" << height_
            << " window" << std::endl;
  std::cout << "[tide::Window] OpenGL " << glGetString(GL_VERSION) << std::endl;

  return true;
}

void Window::destroy() {
  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
  glfwTerminate();
}

void Window::poll_events() { glfwPollEvents(); }

void Window::swap_buffers() {
  if (window_) {
    glfwSwapBuffers(window_);
  }
}

bool Window::should_close() const {
  return window_ && glfwWindowShouldClose(window_);
}

void Window::request_close() {
  if (window_) {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
  }
}

// Static GLFW callbacks

void Window::framebuffer_size_callback(GLFWwindow *window, int width,
                                       int height) {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (self) {
    self->width_ = width;
    self->height_ = height;
    if (self->resize_callback_) {
      self->resize_callback_(width, height);
    }
  }
}

void Window::key_callback(GLFWwindow *window, int key, int scancode, int action,
                          int mods) {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (self) {
    self->keyboard_.on_key(key, scancode, action, mods);
  }
}

void Window::char_callback(GLFWwindow *window, unsigned int codepoint) {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (self) {
    self->keyboard_.on_char(codepoint);
  }
}

void Window::mouse_button_callback(GLFWwindow *window, int button, int action,
                                   int mods) {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (self) {
    self->mouse_.on_button(button, action, mods);
  }
}

void Window::cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (self) {
    self->mouse_.on_move(xpos, ypos);
  }
}

void Window::scroll_callback(GLFWwindow *window, double xoffset,
                             double yoffset) {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (self) {
    self->mouse_.on_scroll(xoffset, yoffset);
  }
}

} // namespace tide::platform::linux
