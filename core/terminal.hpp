#pragma once

#include "core/grid_buffer.hpp"
#include "theme/theme.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace tide::core {

/**
 * Text attributes for terminal cells.
 */
struct Attributes {
  theme::Color foreground;
  theme::Color background;
  bool bold = false;
  bool dim = false;
  bool italic = false;
  bool underline = false;
  bool blink = false;
  bool inverse = false;
  bool hidden = false;
  bool strikethrough = false;

  Attributes() = default;
  explicit Attributes(const theme::Theme &theme)
      : foreground(theme.foreground), background(theme.background) {}
};

/**
 * Terminal emulator - manages grid, cursor, and processes parsed commands.
 */
class Terminal {
public:
  explicit Terminal(int cols = 80, int rows = 24);

  /**
   * Set the color theme.
   */
  void set_theme(const theme::Theme &theme);

  /**
   * Feed raw data from PTY into the terminal.
   * Parses ANSI sequences and updates the grid.
   */
  void feed(const char *data, size_t size);

  /**
   * Resize the terminal.
   */
  void resize(int cols, int rows);

  /**
   * Get the grid buffer for rendering.
   */
  [[nodiscard]] const GridBuffer &grid() const { return grid_; }
  [[nodiscard]] GridBuffer &grid() { return grid_; }

  /**
   * Get cursor position.
   */
  [[nodiscard]] int cursor_col() const { return cursor_col_; }
  [[nodiscard]] int cursor_row() const { return cursor_row_; }

  /**
   * Get dimensions.
   */
  [[nodiscard]] int cols() const { return grid_.cols(); }
  [[nodiscard]] int rows() const { return grid_.rows(); }

  // Scrollback buffer interface
  /**
   * Scroll up through history.
   * @param lines Number of lines to scroll (negative = down)
   */
  void scroll_view(int lines);

  /**
   * Scroll to bottom (live view).
   */
  void scroll_to_bottom();

  /**
   * Get current scroll offset (0 = at bottom/live).
   */
  [[nodiscard]] int scroll_offset() const { return scroll_offset_; }

  /**
   * Get maximum scroll offset (scrollback size).
   */
  [[nodiscard]] int max_scroll() const {
    return static_cast<int>(scrollback_.size());
  }

  /**
   * Check if viewing scrollback (not at bottom).
   */
  [[nodiscard]] bool is_scrolled() const { return scroll_offset_ > 0; }

  /**
   * Get a line for rendering (handles scrollback).
   * @param visual_row Row on screen (0 = top)
   * @return Pointer to cells for that row
   */
  [[nodiscard]] const Cell *get_visible_row(int visual_row) const;

  // Selection interface
  struct Selection {
    int start_col = 0, start_row = 0;
    int end_col = 0, end_row = 0;
    bool active = false;

    // Normalize so start <= end
    void normalize() {
      if (start_row > end_row ||
          (start_row == end_row && start_col > end_col)) {
        std::swap(start_col, end_col);
        std::swap(start_row, end_row);
      }
    }
  };

  /**
   * Start a new selection at given position.
   */
  void start_selection(int col, int row);

  /**
   * Update selection end position.
   */
  void update_selection(int col, int row);

  /**
   * Clear current selection.
   */
  void clear_selection();

  /**
   * Get current selection.
   */
  [[nodiscard]] const Selection &selection() const { return selection_; }

  /**
   * Check if a cell is selected.
   */
  [[nodiscard]] bool is_selected(int col, int row) const;

  /**
   * Get selected text.
   */
  [[nodiscard]] std::string get_selected_text() const;

private:
  // Parser state machine
  enum class State {
    Ground,     // Normal character processing
    Escape,     // After ESC
    CSI_Entry,  // After ESC [
    CSI_Param,  // Collecting CSI parameters
    CSI_Ignore, // Ignoring until final byte
    OSC_String, // Operating System Command
  };

  // Grid and state
  GridBuffer grid_;
  theme::Theme theme_;
  Attributes current_attrs_;
  State state_ = State::Ground;

  // Cursor
  int cursor_col_ = 0;
  int cursor_row_ = 0;

  // Scrollback buffer
  static constexpr int MAX_SCROLLBACK = 10000;
  std::vector<std::vector<Cell>> scrollback_;
  int scroll_offset_ = 0; // 0 = at bottom (live view)

  // Selection
  Selection selection_;

  // CSI parameter collection
  static constexpr int MAX_PARAMS = 16;
  int params_[MAX_PARAMS] = {};
  int param_count_ = 0;
  int current_param_ = 0;
  bool has_param_ = false;
  char intermediate_ = 0; // For '?' in CSI sequences

  // OSC buffer
  std::string osc_buffer_;

  // State machine handlers
  void process_byte(uint8_t byte);
  void state_ground(uint8_t byte);
  void state_escape(uint8_t byte);
  void state_csi_entry(uint8_t byte);
  void state_csi_param(uint8_t byte);
  void state_osc_string(uint8_t byte);

  // Terminal operations
  void print(char32_t codepoint);
  void execute(uint8_t byte); // Control characters
  void csi_dispatch(uint8_t final_byte);

  // Cursor movement
  void cursor_up(int n = 1);
  void cursor_down(int n = 1);
  void cursor_forward(int n = 1);
  void cursor_back(int n = 1);
  void cursor_position(int row, int col);
  void carriage_return();
  void linefeed();
  void reverse_linefeed();

  // Erase operations
  void erase_display(int mode);
  void erase_line(int mode);
  void erase_chars(int n);
  void delete_chars(int n);
  void insert_chars(int n);

  // Scrolling
  void scroll_up(int n = 1);
  void scroll_down(int n = 1);

  // SGR (Select Graphic Rendition)
  void select_graphic_rendition();

  // Helpers
  void reset_params();
  int get_param(int index, int default_value = 0) const;
  void clamp_cursor();
  Cell make_cell(char32_t codepoint) const;
};

} // namespace tide::core
