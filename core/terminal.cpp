#include "core/terminal.hpp"

#include <algorithm>
#include <iostream>

namespace tide::core {

Terminal::Terminal(int cols, int rows)
    : grid_(cols, rows), theme_(theme::get_default_theme()),
      current_attrs_(theme_) {}

void Terminal::set_theme(const theme::Theme &theme) {
  theme_ = theme;
  current_attrs_.foreground = theme.foreground;
  current_attrs_.background = theme.background;
}

void Terminal::feed(const char *data, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    process_byte(static_cast<uint8_t>(data[i]));
  }
}

void Terminal::resize(int cols, int rows) {
  grid_.resize(cols, rows);
  clamp_cursor();
}

void Terminal::process_byte(uint8_t byte) {
  switch (state_) {
  case State::Ground:
    state_ground(byte);
    break;
  case State::Escape:
    state_escape(byte);
    break;
  case State::CSI_Entry:
    state_csi_entry(byte);
    break;
  case State::CSI_Param:
    state_csi_param(byte);
    break;
  case State::CSI_Ignore:
    // Wait for final byte (0x40-0x7E)
    if (byte >= 0x40 && byte <= 0x7E) {
      state_ = State::Ground;
    }
    break;
  case State::OSC_String:
    state_osc_string(byte);
    break;
  }
}

void Terminal::state_ground(uint8_t byte) {
  if (byte == 0x1B) {
    // ESC - start escape sequence
    state_ = State::Escape;
  } else if (byte < 0x20) {
    // Control character
    execute(byte);
  } else if (byte >= 0x20 && byte < 0x7F) {
    // Printable ASCII
    print(static_cast<char32_t>(byte));
  } else if (byte >= 0x80) {
    // TODO: Handle UTF-8 multi-byte sequences
    // For now, just print as-is
    print(static_cast<char32_t>(byte));
  }
}

void Terminal::state_escape(uint8_t byte) {
  if (byte == '[') {
    // CSI - Control Sequence Introducer
    state_ = State::CSI_Entry;
    reset_params();
  } else if (byte == ']') {
    // OSC - Operating System Command
    state_ = State::OSC_String;
    osc_buffer_.clear();
  } else if (byte == 'M') {
    // Reverse linefeed
    reverse_linefeed();
    state_ = State::Ground;
  } else if (byte == 'D') {
    // Linefeed
    linefeed();
    state_ = State::Ground;
  } else if (byte == 'E') {
    // Newline
    carriage_return();
    linefeed();
    state_ = State::Ground;
  } else if (byte == 'c') {
    // Reset terminal
    grid_.clear();
    cursor_col_ = 0;
    cursor_row_ = 0;
    current_attrs_ = Attributes(theme_);
    state_ = State::Ground;
  } else if (byte == '7') {
    // Save cursor (DECSC) - TODO
    state_ = State::Ground;
  } else if (byte == '8') {
    // Restore cursor (DECRC) - TODO
    state_ = State::Ground;
  } else {
    // Unknown escape sequence, return to ground
    state_ = State::Ground;
  }
}

void Terminal::state_csi_entry(uint8_t byte) {
  if (byte == '?') {
    // Private mode indicator
    intermediate_ = '?';
    state_ = State::CSI_Param;
  } else if (byte >= '0' && byte <= '9') {
    current_param_ = byte - '0';
    has_param_ = true;
    state_ = State::CSI_Param;
  } else if (byte == ';') {
    // Empty first parameter
    params_[param_count_++] = 0;
    state_ = State::CSI_Param;
  } else if (byte >= 0x40 && byte <= 0x7E) {
    // Final byte with no parameters
    csi_dispatch(byte);
    state_ = State::Ground;
  } else {
    state_ = State::CSI_Ignore;
  }
}

void Terminal::state_csi_param(uint8_t byte) {
  if (byte >= '0' && byte <= '9') {
    current_param_ = current_param_ * 10 + (byte - '0');
    has_param_ = true;
  } else if (byte == ';') {
    // Parameter separator
    if (param_count_ < MAX_PARAMS) {
      params_[param_count_++] = has_param_ ? current_param_ : 0;
    }
    current_param_ = 0;
    has_param_ = false;
  } else if (byte >= 0x40 && byte <= 0x7E) {
    // Final byte
    if (has_param_ && param_count_ < MAX_PARAMS) {
      params_[param_count_++] = current_param_;
    }
    csi_dispatch(byte);
    state_ = State::Ground;
  } else if (byte >= 0x20 && byte <= 0x2F) {
    // Intermediate byte
    intermediate_ = static_cast<char>(byte);
  } else {
    state_ = State::CSI_Ignore;
  }
}

void Terminal::state_osc_string(uint8_t byte) {
  if (byte == 0x07) {
    // BEL terminates OSC
    // TODO: Process OSC command (e.g., set window title)
    state_ = State::Ground;
  } else if (byte == 0x1B) {
    // Might be ESC \ (ST)
    // TODO: Handle properly
    state_ = State::Ground;
  } else {
    osc_buffer_ += static_cast<char>(byte);
  }
}

void Terminal::execute(uint8_t byte) {
  switch (byte) {
  case 0x07: // BEL - Bell
    // TODO: Visual bell or sound
    break;
  case 0x08: // BS - Backspace
    cursor_back(1);
    break;
  case 0x09: // HT - Horizontal Tab
    cursor_col_ = ((cursor_col_ / 8) + 1) * 8;
    if (cursor_col_ >= grid_.cols()) {
      cursor_col_ = grid_.cols() - 1;
    }
    break;
  case 0x0A: // LF - Line Feed
  case 0x0B: // VT - Vertical Tab
  case 0x0C: // FF - Form Feed
    linefeed();
    break;
  case 0x0D: // CR - Carriage Return
    carriage_return();
    break;
  default:
    break;
  }
}

void Terminal::csi_dispatch(uint8_t final_byte) {
  switch (final_byte) {
  case 'A': // CUU - Cursor Up
    cursor_up(get_param(0, 1));
    break;
  case 'B': // CUD - Cursor Down
    cursor_down(get_param(0, 1));
    break;
  case 'C': // CUF - Cursor Forward
    cursor_forward(get_param(0, 1));
    break;
  case 'D': // CUB - Cursor Back
    cursor_back(get_param(0, 1));
    break;
  case 'E': // CNL - Cursor Next Line
    cursor_down(get_param(0, 1));
    carriage_return();
    break;
  case 'F': // CPL - Cursor Previous Line
    cursor_up(get_param(0, 1));
    carriage_return();
    break;
  case 'G': // CHA - Cursor Horizontal Absolute
    cursor_col_ = get_param(0, 1) - 1;
    clamp_cursor();
    break;
  case 'H': // CUP - Cursor Position
  case 'f': // HVP - Horizontal Vertical Position
    cursor_position(get_param(0, 1), get_param(1, 1));
    break;
  case 'J': // ED - Erase Display
    erase_display(get_param(0, 0));
    break;
  case 'K': // EL - Erase Line
    erase_line(get_param(0, 0));
    break;
  case 'L': // IL - Insert Lines
    // TODO: Insert blank lines
    break;
  case 'M': // DL - Delete Lines
    // TODO: Delete lines
    break;
  case 'P': // DCH - Delete Characters
    delete_chars(get_param(0, 1));
    break;
  case 'S': // SU - Scroll Up
    scroll_up(get_param(0, 1));
    break;
  case 'T': // SD - Scroll Down
    scroll_down(get_param(0, 1));
    break;
  case 'X': // ECH - Erase Characters
    erase_chars(get_param(0, 1));
    break;
  case '@': // ICH - Insert Characters
    insert_chars(get_param(0, 1));
    break;
  case 'd': // VPA - Vertical Position Absolute
    cursor_row_ = get_param(0, 1) - 1;
    clamp_cursor();
    break;
  case 'm': // SGR - Select Graphic Rendition
    select_graphic_rendition();
    break;
  case 'h': // SM / DECSET - Set Mode
    // TODO: Handle modes
    break;
  case 'l': // RM / DECRST - Reset Mode
    // TODO: Handle modes
    break;
  case 'r': // DECSTBM - Set Top and Bottom Margins
    // TODO: Scrolling region
    break;
  case 's': // SCP - Save Cursor Position
    // TODO: Save cursor
    break;
  case 'u': // RCP - Restore Cursor Position
    // TODO: Restore cursor
    break;
  default:
    // Unknown CSI sequence
    break;
  }
}

void Terminal::print(char32_t codepoint) {
  if (cursor_col_ >= grid_.cols()) {
    // Line wrap
    carriage_return();
    linefeed();
  }

  grid_.set(cursor_col_, cursor_row_, make_cell(codepoint));
  cursor_col_++;
}

void Terminal::cursor_up(int n) { cursor_row_ = std::max(0, cursor_row_ - n); }

void Terminal::cursor_down(int n) {
  cursor_row_ = std::min(grid_.rows() - 1, cursor_row_ + n);
}

void Terminal::cursor_forward(int n) {
  cursor_col_ = std::min(grid_.cols() - 1, cursor_col_ + n);
}

void Terminal::cursor_back(int n) {
  cursor_col_ = std::max(0, cursor_col_ - n);
}

void Terminal::cursor_position(int row, int col) {
  // CSI coordinates are 1-based
  cursor_row_ = row - 1;
  cursor_col_ = col - 1;
  clamp_cursor();
}

void Terminal::carriage_return() { cursor_col_ = 0; }

void Terminal::linefeed() {
  if (cursor_row_ < grid_.rows() - 1) {
    cursor_row_++;
  } else {
    // At bottom, scroll up
    scroll_up(1);
  }
}

void Terminal::reverse_linefeed() {
  if (cursor_row_ > 0) {
    cursor_row_--;
  } else {
    // At top, scroll down
    scroll_down(1);
  }
}

void Terminal::erase_display(int mode) {
  Cell blank = make_cell(U' ');

  switch (mode) {
  case 0: // Erase from cursor to end of screen
    erase_line(0);
    for (int row = cursor_row_ + 1; row < grid_.rows(); ++row) {
      grid_.clear_row(row, blank);
    }
    break;
  case 1: // Erase from start of screen to cursor
    for (int row = 0; row < cursor_row_; ++row) {
      grid_.clear_row(row, blank);
    }
    erase_line(1);
    break;
  case 2: // Erase entire screen
  case 3: // Erase entire screen + scrollback
    grid_.clear(blank);
    break;
  }
}

void Terminal::erase_line(int mode) {
  Cell blank = make_cell(U' ');

  switch (mode) {
  case 0: // Erase from cursor to end of line
    for (int col = cursor_col_; col < grid_.cols(); ++col) {
      grid_.set(col, cursor_row_, blank);
    }
    break;
  case 1: // Erase from start of line to cursor
    for (int col = 0; col <= cursor_col_; ++col) {
      grid_.set(col, cursor_row_, blank);
    }
    break;
  case 2: // Erase entire line
    grid_.clear_row(cursor_row_, blank);
    break;
  }
}

void Terminal::erase_chars(int n) {
  Cell blank = make_cell(U' ');
  int end = std::min(cursor_col_ + n, grid_.cols());
  for (int col = cursor_col_; col < end; ++col) {
    grid_.set(col, cursor_row_, blank);
  }
}

void Terminal::delete_chars(int n) {
  int cols = grid_.cols();
  int end = std::min(cursor_col_ + n, cols);

  // Shift characters left
  for (int col = cursor_col_; col < cols - n; ++col) {
    grid_.set(col, cursor_row_, grid_.at(col + n, cursor_row_));
  }

  // Fill end with blanks
  Cell blank = make_cell(U' ');
  for (int col = cols - n; col < cols; ++col) {
    grid_.set(col, cursor_row_, blank);
  }
}

void Terminal::insert_chars(int n) {
  int cols = grid_.cols();

  // Shift characters right
  for (int col = cols - 1; col >= cursor_col_ + n; --col) {
    grid_.set(col, cursor_row_, grid_.at(col - n, cursor_row_));
  }

  // Fill with blanks
  Cell blank = make_cell(U' ');
  int end = std::min(cursor_col_ + n, cols);
  for (int col = cursor_col_; col < end; ++col) {
    grid_.set(col, cursor_row_, blank);
  }
}

void Terminal::scroll_up(int n) {
  Cell blank = make_cell(U' ');
  int rows = grid_.rows();
  int cols = grid_.cols();

  // Save top lines to scrollback before discarding
  for (int i = 0; i < n && i < rows; ++i) {
    std::vector<Cell> line(cols);
    for (int col = 0; col < cols; ++col) {
      line[col] = grid_.at(col, i);
    }
    scrollback_.push_back(std::move(line));

    // Trim scrollback if too large
    if (static_cast<int>(scrollback_.size()) > MAX_SCROLLBACK) {
      scrollback_.erase(scrollback_.begin());
    }
  }

  // Move lines up
  for (int row = 0; row < rows - n; ++row) {
    for (int col = 0; col < cols; ++col) {
      grid_.set(col, row, grid_.at(col, row + n));
    }
  }

  // Clear bottom lines
  for (int row = rows - n; row < rows; ++row) {
    grid_.clear_row(row, blank);
  }
}

void Terminal::scroll_down(int n) {
  Cell blank = make_cell(U' ');
  int rows = grid_.rows();
  int cols = grid_.cols();

  // Move lines down
  for (int row = rows - 1; row >= n; --row) {
    for (int col = 0; col < cols; ++col) {
      grid_.set(col, row, grid_.at(col, row - n));
    }
  }

  // Clear top lines
  for (int row = 0; row < n; ++row) {
    grid_.clear_row(row, blank);
  }
}

void Terminal::select_graphic_rendition() {
  if (param_count_ == 0) {
    // No params means reset
    current_attrs_ = Attributes(theme_);
    return;
  }

  for (int i = 0; i < param_count_; ++i) {
    int code = params_[i];

    switch (code) {
    case 0: // Reset
      current_attrs_ = Attributes(theme_);
      break;
    case 1: // Bold
      current_attrs_.bold = true;
      break;
    case 2: // Dim
      current_attrs_.dim = true;
      break;
    case 3: // Italic
      current_attrs_.italic = true;
      break;
    case 4: // Underline
      current_attrs_.underline = true;
      break;
    case 5: // Blink
      current_attrs_.blink = true;
      break;
    case 7: // Inverse
      current_attrs_.inverse = true;
      break;
    case 8: // Hidden
      current_attrs_.hidden = true;
      break;
    case 9: // Strikethrough
      current_attrs_.strikethrough = true;
      break;
    case 21: // Bold off (or double underline)
    case 22: // Normal intensity
      current_attrs_.bold = false;
      current_attrs_.dim = false;
      break;
    case 23: // Italic off
      current_attrs_.italic = false;
      break;
    case 24: // Underline off
      current_attrs_.underline = false;
      break;
    case 25: // Blink off
      current_attrs_.blink = false;
      break;
    case 27: // Inverse off
      current_attrs_.inverse = false;
      break;
    case 28: // Hidden off
      current_attrs_.hidden = false;
      break;
    case 29: // Strikethrough off
      current_attrs_.strikethrough = false;
      break;

    // Foreground colors (30-37)
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
      current_attrs_.foreground = theme_.ansi_colors[code - 30];
      break;

    case 38: // Extended foreground
      if (i + 2 < param_count_ && params_[i + 1] == 5) {
        // 256-color mode: 38;5;n
        int color_idx = params_[i + 2];
        if (color_idx < 16) {
          current_attrs_.foreground = theme_.ansi_colors[color_idx];
        }
        // TODO: Handle 16-255 color palette
        i += 2;
      } else if (i + 4 < param_count_ && params_[i + 1] == 2) {
        // RGB mode: 38;2;r;g;b
        float r = params_[i + 2] / 255.0f;
        float g = params_[i + 3] / 255.0f;
        float b = params_[i + 4] / 255.0f;
        current_attrs_.foreground = theme::Color(r, g, b);
        i += 4;
      }
      break;

    case 39: // Default foreground
      current_attrs_.foreground = theme_.foreground;
      break;

    // Background colors (40-47)
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
    case 47:
      current_attrs_.background = theme_.ansi_colors[code - 40];
      break;

    case 48: // Extended background
      if (i + 2 < param_count_ && params_[i + 1] == 5) {
        // 256-color mode: 48;5;n
        int color_idx = params_[i + 2];
        if (color_idx < 16) {
          current_attrs_.background = theme_.ansi_colors[color_idx];
        }
        i += 2;
      } else if (i + 4 < param_count_ && params_[i + 1] == 2) {
        // RGB mode: 48;2;r;g;b
        float r = params_[i + 2] / 255.0f;
        float g = params_[i + 3] / 255.0f;
        float b = params_[i + 4] / 255.0f;
        current_attrs_.background = theme::Color(r, g, b);
        i += 4;
      }
      break;

    case 49: // Default background
      current_attrs_.background = theme_.background;
      break;

    // Bright foreground (90-97)
    case 90:
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
    case 96:
    case 97:
      current_attrs_.foreground = theme_.ansi_colors[code - 90 + 8];
      break;

    // Bright background (100-107)
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
      current_attrs_.background = theme_.ansi_colors[code - 100 + 8];
      break;

    default:
      break;
    }
  }
}

void Terminal::reset_params() {
  param_count_ = 0;
  current_param_ = 0;
  has_param_ = false;
  intermediate_ = 0;
  std::fill(std::begin(params_), std::end(params_), 0);
}

int Terminal::get_param(int index, int default_value) const {
  if (index < param_count_ && params_[index] != 0) {
    return params_[index];
  }
  return default_value;
}

void Terminal::clamp_cursor() {
  cursor_col_ = std::clamp(cursor_col_, 0, grid_.cols() - 1);
  cursor_row_ = std::clamp(cursor_row_, 0, grid_.rows() - 1);
}

Cell Terminal::make_cell(char32_t codepoint) const {
  theme::Color fg = current_attrs_.foreground;
  theme::Color bg = current_attrs_.background;

  if (current_attrs_.inverse) {
    std::swap(fg, bg);
  }

  return Cell(codepoint, fg, bg);
}

// Scrollback methods
void Terminal::scroll_view(int lines) {
  scroll_offset_ += lines;

  // Clamp to valid range
  int max_offset = static_cast<int>(scrollback_.size());
  scroll_offset_ = std::clamp(scroll_offset_, 0, max_offset);
}

void Terminal::scroll_to_bottom() { scroll_offset_ = 0; }

const Cell *Terminal::get_visible_row(int visual_row) const {
  int scrollback_size = static_cast<int>(scrollback_.size());
  int rows = grid_.rows();

  // Calculate which row to display
  // scroll_offset_ = how many lines we've scrolled up into history
  // visual_row 0 is top of screen

  if (scroll_offset_ == 0) {
    // Not scrolled, show live grid
    if (visual_row >= 0 && visual_row < rows) {
      return &grid_.at(0, visual_row);
    }
    return nullptr;
  }

  // We're scrolled into history
  // The view shows: some scrollback lines at top, possibly some grid lines at
  // bottom
  int scrollback_lines_shown = std::min(scroll_offset_, rows);
  int scrollback_start = scrollback_size - scroll_offset_;

  if (visual_row < scrollback_lines_shown) {
    // This row comes from scrollback
    int scrollback_idx = scrollback_start + visual_row;
    if (scrollback_idx >= 0 && scrollback_idx < scrollback_size) {
      return scrollback_[scrollback_idx].data();
    }
    return nullptr;
  } else {
    // This row comes from current grid
    int grid_row = visual_row - scrollback_lines_shown;
    if (grid_row >= 0 && grid_row < rows) {
      return &grid_.at(0, grid_row);
    }
    return nullptr;
  }
}

// Selection methods
void Terminal::start_selection(int col, int row) {
  selection_.start_col = col;
  selection_.start_row = row;
  selection_.end_col = col;
  selection_.end_row = row;
  selection_.active = true;
}

void Terminal::update_selection(int col, int row) {
  if (!selection_.active)
    return;
  selection_.end_col = col;
  selection_.end_row = row;
}

void Terminal::clear_selection() {
  selection_.active = false;
  selection_.start_col = selection_.start_row = 0;
  selection_.end_col = selection_.end_row = 0;
}

bool Terminal::is_selected(int col, int row) const {
  if (!selection_.active)
    return false;

  Selection sel = selection_;
  sel.normalize();

  // Check if row is within selection range
  if (row < sel.start_row || row > sel.end_row)
    return false;

  if (sel.start_row == sel.end_row) {
    // Single line selection
    return col >= sel.start_col && col <= sel.end_col;
  }

  if (row == sel.start_row) {
    // First line: from start_col to end of line
    return col >= sel.start_col;
  }

  if (row == sel.end_row) {
    // Last line: from start to end_col
    return col <= sel.end_col;
  }

  // Middle lines: fully selected
  return true;
}

std::string Terminal::get_selected_text() const {
  if (!selection_.active)
    return "";

  Selection sel = selection_;
  sel.normalize();

  std::string result;
  int cols = grid_.cols();

  for (int row = sel.start_row; row <= sel.end_row; ++row) {
    int start_col = (row == sel.start_row) ? sel.start_col : 0;
    int end_col = (row == sel.end_row) ? sel.end_col : cols - 1;

    for (int col = start_col; col <= end_col; ++col) {
      const Cell &cell = grid_.at(col, row);
      // Simple ASCII conversion for now
      if (cell.codepoint < 128 && cell.codepoint >= 32) {
        result += static_cast<char>(cell.codepoint);
      } else if (cell.codepoint == U' ' || cell.codepoint == 0) {
        result += ' ';
      }
    }

    // Add newline between lines (but not after last)
    if (row < sel.end_row) {
      // Trim trailing spaces before newline
      while (!result.empty() && result.back() == ' ') {
        result.pop_back();
      }
      result += '\n';
    }
  }

  return result;
}

} // namespace tide::core
