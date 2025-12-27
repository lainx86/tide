/**
 * tide - A modern GPU-accelerated terminal emulator
 *
 * Main entry point and application event loop.
 */

#include "core/ansi_parser.hpp"
#include "core/grid_buffer.hpp"
#include "core/pty.hpp"
#include "platform/linux/window.hpp"
#include "render/font.hpp"
#include "render/renderer.hpp"
#include "theme/theme.hpp"


#include <cstring>
#include <iostream>


namespace {

// Configuration constants
constexpr int DEFAULT_WINDOW_WIDTH = 800;
constexpr int DEFAULT_WINDOW_HEIGHT = 600;
constexpr int DEFAULT_COLS = 80;
constexpr int DEFAULT_ROWS = 24;
constexpr size_t PTY_READ_BUFFER_SIZE = 4096;

} // anonymous namespace

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  std::cout << "tide - Terminal Emulator v0.1.0" << std::endl;
  std::cout << "================================" << std::endl;

  // Initialize theme
  tide::theme::Theme theme = tide::theme::get_default_theme();
  std::cout << "[tide] Using theme: " << theme.name << std::endl;

  // Create window
  tide::platform::linux::Window window;
  if (!window.create(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, "tide")) {
    std::cerr << "[tide] Failed to create window" << std::endl;
    return 1;
  }

  // Initialize font system
  tide::render::Font font;
  if (!font.init()) {
    std::cerr << "[tide] Failed to initialize font system" << std::endl;
    return 1;
  }

  // TODO: Load actual font
  // font.load("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14);

  // Initialize renderer
  tide::render::Renderer renderer;
  if (!renderer.init()) {
    std::cerr << "[tide] Failed to initialize renderer" << std::endl;
    return 1;
  }
  renderer.resize(window.width(), window.height());
  renderer.set_theme(theme);

  // Create terminal grid
  tide::core::GridBuffer grid(DEFAULT_COLS, DEFAULT_ROWS);

  // Create ANSI parser
  tide::core::AnsiParser parser;

  // Spawn PTY with shell
  tide::core::Pty pty;
  if (!pty.spawn()) {
    std::cerr << "[tide] Failed to spawn PTY" << std::endl;
    return 1;
  }

  // Set initial PTY size
  pty.resize(DEFAULT_COLS, DEFAULT_ROWS);

  // Set up window callbacks
  window.set_resize_callback([&](int width, int height) {
    renderer.resize(width, height);

    // TODO: Recalculate grid dimensions based on font metrics
    // int new_cols = width / font.cell_width();
    // int new_rows = height / font.cell_height();
    // grid.resize(new_cols, new_rows);
    // pty.resize(new_cols, new_rows);
  });

  // Set up keyboard input - forward to PTY
  window.keyboard().set_char_callback([&](unsigned int codepoint) {
    // Simple ASCII passthrough for now
    if (codepoint < 128) {
      char c = static_cast<char>(codepoint);
      pty.write(&c, 1);
    }
    // TODO: Handle UTF-8 encoding for non-ASCII
  });

  window.keyboard().set_key_callback(
      [&](int key, int scancode, int action, int mods) {
        (void)scancode;

        if (action != 1 /* GLFW_PRESS */ && action != 2 /* GLFW_REPEAT */) {
          return;
        }

        // Handle special keys
        // TODO: Proper key translation with modifier handling

        // Ctrl+C, Ctrl+D, etc.
        if (mods & 0x0002 /* GLFW_MOD_CONTROL */) {
          if (key >= 'A' && key <= 'Z') {
            char ctrl_char = static_cast<char>(key - 'A' + 1);
            pty.write(&ctrl_char, 1);
          }
        }

        // Enter key
        if (key == 257 /* GLFW_KEY_ENTER */) {
          char newline = '\n';
          pty.write(&newline, 1);
        }

        // Backspace
        if (key == 259 /* GLFW_KEY_BACKSPACE */) {
          char backspace = '\x7f';
          pty.write(&backspace, 1);
        }

        // TODO: Arrow keys, function keys, etc.
      });

  // PTY read buffer
  char pty_buffer[PTY_READ_BUFFER_SIZE];

  std::cout << "[tide] Entering main loop..." << std::endl;

  // Main event loop
  while (!window.should_close()) {
    // Poll window events
    window.poll_events();

    // Read from PTY (non-blocking)
    ssize_t bytes_read = pty.read(pty_buffer, PTY_READ_BUFFER_SIZE);
    if (bytes_read > 0) {
      // Feed data to ANSI parser
      parser.feed(pty_buffer, static_cast<size_t>(bytes_read));

      // TODO: Parser should emit events that update the grid
    } else if (bytes_read < 0) {
      // PTY closed or error
      std::cout << "[tide] PTY closed, exiting..." << std::endl;
      break;
    }

    // Render frame
    renderer.render(grid, theme);

    // Swap buffers
    window.swap_buffers();
  }

  // Cleanup
  std::cout << "[tide] Shutting down..." << std::endl;
  pty.close();
  renderer.shutdown();
  font.shutdown();
  window.destroy();

  return 0;
}
