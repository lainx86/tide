#pragma once

#include "theme/theme.hpp"

#include <cstdint>
#include <vector>


namespace tide::core {

/**
 * A single cell in the terminal grid.
 */
struct Cell {
  char32_t codepoint = U' '; // Unicode codepoint (space by default)
  theme::Color foreground;   // Foreground color
  theme::Color background;   // Background color

  // TODO: Add text attributes
  // bool bold = false;
  // bool italic = false;
  // bool underline = false;
  // bool strikethrough = false;
  // bool blink = false;
  // bool inverse = false;

  Cell() = default;
  Cell(char32_t cp, theme::Color fg, theme::Color bg)
      : codepoint(cp), foreground(fg), background(bg) {}
};

/**
 * Grid buffer representing the terminal's character grid.
 * Stores cells arranged in rows and columns.
 */
class GridBuffer {
public:
  /**
   * Create a grid buffer with specified dimensions.
   * @param cols Number of columns
   * @param rows Number of rows
   */
  GridBuffer(int cols = 80, int rows = 24);

  /**
   * Resize the grid buffer.
   * Preserves existing content where possible.
   * @param new_cols New number of columns
   * @param new_rows New number of rows
   *
   * TODO: Handle content reflow on resize
   */
  void resize(int new_cols, int new_rows);

  /**
   * Clear the entire grid with a single cell value.
   */
  void clear(const Cell &cell = Cell{});

  /**
   * Clear a specific row.
   */
  void clear_row(int row, const Cell &cell = Cell{});

  /**
   * Get a cell at the specified position.
   * @return Reference to the cell
   */
  [[nodiscard]] Cell &at(int col, int row);
  [[nodiscard]] const Cell &at(int col, int row) const;

  /**
   * Set a cell at the specified position.
   */
  void set(int col, int row, const Cell &cell);

  /**
   * Set a character at the specified position using default colors.
   */
  void set_char(int col, int row, char32_t codepoint);

  // Getters
  [[nodiscard]] int cols() const { return cols_; }
  [[nodiscard]] int rows() const { return rows_; }

  // TODO: Add scrollback buffer support
  // TODO: Add dirty region tracking for efficient rendering

private:
  int cols_;
  int rows_;
  std::vector<Cell> cells_;

  [[nodiscard]] size_t index(int col, int row) const {
    return static_cast<size_t>(row * cols_ + col);
  }

  [[nodiscard]] bool valid(int col, int row) const {
    return col >= 0 && col < cols_ && row >= 0 && row < rows_;
  }
};

} // namespace tide::core
