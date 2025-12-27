/**
 * tide - A modern GPU-accelerated terminal emulator
 *
 * Main entry point and application event loop.
 */

#include "core/pty.hpp"
#include "core/terminal.hpp"
#include "platform/linux/window.hpp"
#include "render/font.hpp"
#include "render/renderer.hpp"
#include "theme/theme.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>

namespace {

// Configuration constants
constexpr int DEFAULT_WINDOW_WIDTH = 900;
constexpr int DEFAULT_WINDOW_HEIGHT = 600;
constexpr int DEFAULT_FONT_SIZE = 16;
constexpr size_t PTY_READ_BUFFER_SIZE = 4096;

// Common font paths to try
const char *FONT_PATHS[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    "/usr/share/fonts/liberation-mono/LiberationMono-Regular.ttf",
    nullptr};

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

  // Try to load a font
  bool font_loaded = false;
  for (int i = 0; FONT_PATHS[i] != nullptr; ++i) {
    if (font.load(FONT_PATHS[i], DEFAULT_FONT_SIZE)) {
      font_loaded = true;
      break;
    }
  }

  if (!font_loaded) {
    std::cerr
        << "[tide] Failed to load any font! Please install a monospace font."
        << std::endl;
    std::cerr << "[tide] Try: sudo apt install fonts-dejavu-core" << std::endl;
    return 1;
  }

  // Initialize renderer with font
  tide::render::Renderer renderer;
  if (!renderer.init(&font)) {
    std::cerr << "[tide] Failed to initialize renderer" << std::endl;
    return 1;
  }
  renderer.resize(window.width(), window.height());
  renderer.set_theme(theme);

  // Calculate grid size from window and font
  int cols = renderer.grid_cols();
  int rows = renderer.grid_rows();
  std::cout << "[tide] Grid size: " << cols << "x" << rows << std::endl;

  // Create terminal
  tide::core::Terminal terminal(cols, rows);
  terminal.set_theme(theme);

  // Spawn PTY with shell
  tide::core::Pty pty;
  if (!pty.spawn()) {
    std::cerr << "[tide] Failed to spawn PTY" << std::endl;
    return 1;
  }

  // Set initial PTY size
  pty.resize(cols, rows);

  // Set up window callbacks
  window.set_resize_callback([&](int width, int height) {
    renderer.resize(width, height);

    // Recalculate grid dimensions
    int new_cols = renderer.grid_cols();
    int new_rows = renderer.grid_rows();

    if (new_cols != terminal.cols() || new_rows != terminal.rows()) {
      terminal.resize(new_cols, new_rows);
      pty.resize(new_cols, new_rows);
      std::cout << "[tide] Resized to: " << new_cols << "x" << new_rows
                << std::endl;
    }
  });

  // Mouse state for selection
  bool mouse_selecting = false;
  double last_mouse_x = 0, last_mouse_y = 0;

  // Helper to convert mouse coords to cell position
  auto mouse_to_cell = [&](double x, double y, int &col, int &row) {
    col = static_cast<int>(x) / font.cell_width();
    row = static_cast<int>(y) / font.cell_height();
    col = std::clamp(col, 0, terminal.cols() - 1);
    row = std::clamp(row, 0, terminal.rows() - 1);
  };

  // Set up mouse button callback for selection
  window.mouse().set_button_callback([&](int button, int action, int mods) {
    (void)mods;
    if (button == 0) {   // Left button
      if (action == 1) { // Press
        int col, row;
        mouse_to_cell(last_mouse_x, last_mouse_y, col, row);
        terminal.start_selection(col, row);
        mouse_selecting = true;
      } else if (action == 0) { // Release
        if (mouse_selecting && terminal.selection().active) {
          // Copy to clipboard
          std::string text = terminal.get_selected_text();
          if (!text.empty()) {
            glfwSetClipboardString(nullptr, text.c_str());
          }
        }
        mouse_selecting = false;
      }
    }
  });

  // Set up mouse motion callback
  window.mouse().set_move_callback([&](double x, double y) {
    last_mouse_x = x;
    last_mouse_y = y;
    if (mouse_selecting) {
      int col, row;
      mouse_to_cell(x, y, col, row);
      terminal.update_selection(col, row);
    }
  });

  // Set up mouse scroll for scrollback
  window.mouse().set_scroll_callback([&](double xoffset, double yoffset) {
    (void)xoffset;
    int lines = static_cast<int>(yoffset * 3); // 3 lines per scroll step
    terminal.scroll_view(lines);
  });

  // Set up keyboard input - forward to PTY
  window.keyboard().set_char_callback([&](unsigned int codepoint) {
    terminal.scroll_to_bottom(); // Auto-scroll on input
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

        // Ctrl+C, Ctrl+D, etc.
        if (mods & 0x0002 /* GLFW_MOD_CONTROL */) {
          if (key >= 'A' && key <= 'Z') {
            char ctrl_char = static_cast<char>(key - 'A' + 1);
            pty.write(&ctrl_char, 1);
            return;
          }
        }

        // Arrow keys - send ANSI escape sequences
        const char *seq = nullptr;
        switch (key) {
        case 262:
          seq = "\x1b[C";
          break; // Right
        case 263:
          seq = "\x1b[D";
          break; // Left
        case 264:
          seq = "\x1b[B";
          break; // Down
        case 265:
          seq = "\x1b[A";
          break; // Up
        case 268:
          seq = "\x1b[H";
          break; // Home
        case 269:
          seq = "\x1b[F";
          break; // End
        case 266:
          seq = "\x1b[5~";
          break; // Page Up
        case 267:
          seq = "\x1b[6~";
          break; // Page Down
        case 261:
          seq = "\x1b[3~";
          break; // Delete
        default:
          break;
        }

        if (seq) {
          pty.write(seq, strlen(seq));
          return;
        }

        // Enter key
        if (key == 257 /* GLFW_KEY_ENTER */) {
          char newline = '\n';
          pty.write(&newline, 1);
          return;
        }

        // Backspace
        if (key == 259 /* GLFW_KEY_BACKSPACE */) {
          char backspace = '\x7f';
          pty.write(&backspace, 1);
          return;
        }

        // Tab
        if (key == 258 /* GLFW_KEY_TAB */) {
          char tab = '\t';
          pty.write(&tab, 1);
          return;
        }

        // Escape
        if (key == 256 /* GLFW_KEY_ESCAPE */) {
          char esc = '\x1b';
          pty.write(&esc, 1);
          return;
        }
        terminal.scroll_to_bottom(); // Auto-scroll on key input
      });

  // PTY read buffer
  char pty_buffer[PTY_READ_BUFFER_SIZE];

  // Cursor blink state
  auto last_blink_time = std::chrono::steady_clock::now();
  bool cursor_visible = true;
  constexpr auto BLINK_INTERVAL = std::chrono::milliseconds(500);

  std::cout << "[tide] Entering main loop..." << std::endl;

  // Main event loop
  while (!window.should_close()) {
    // Poll window events
    window.poll_events();

    // Read from PTY (non-blocking)
    ssize_t bytes_read = pty.read(pty_buffer, PTY_READ_BUFFER_SIZE);
    if (bytes_read > 0) {
      // Feed data to terminal (parses ANSI and updates grid)
      terminal.feed(pty_buffer, static_cast<size_t>(bytes_read));
      // Reset cursor blink on output
      cursor_visible = true;
      last_blink_time = std::chrono::steady_clock::now();
    } else if (bytes_read < 0) {
      // PTY closed or error
      std::cout << "[tide] PTY closed, exiting..." << std::endl;
      break;
    }

    // Update cursor blink
    auto now = std::chrono::steady_clock::now();
    if (now - last_blink_time >= BLINK_INTERVAL) {
      cursor_visible = !cursor_visible;
      last_blink_time = now;
    }

    // Hide cursor if scrolled into history
    bool show_cursor = cursor_visible && !terminal.is_scrolled();

    // Set selection check for highlighting
    renderer.set_selection_check(
        [&](int col, int row) { return terminal.is_selected(col, row); });

    // Render frame with cursor
    renderer.render(terminal.grid(), theme, terminal.cursor_col(),
                    terminal.cursor_row(), show_cursor);

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
