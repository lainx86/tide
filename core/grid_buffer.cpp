#include "core/grid_buffer.hpp"

#include <algorithm>
#include <stdexcept>


namespace tide::core {

GridBuffer::GridBuffer(int cols, int rows)
    : cols_(cols), rows_(rows), cells_(static_cast<size_t>(cols * rows)) {
  clear();
}

void GridBuffer::resize(int new_cols, int new_rows) {
  if (new_cols <= 0 || new_rows <= 0) {
    return;
  }

  if (new_cols == cols_ && new_rows == rows_) {
    return;
  }

  // TODO: Implement proper content reflow
  // For now, create new buffer and copy what fits

  std::vector<Cell> new_cells(static_cast<size_t>(new_cols * new_rows));

  int copy_cols = std::min(cols_, new_cols);
  int copy_rows = std::min(rows_, new_rows);

  for (int row = 0; row < copy_rows; ++row) {
    for (int col = 0; col < copy_cols; ++col) {
      size_t old_idx = static_cast<size_t>(row * cols_ + col);
      size_t new_idx = static_cast<size_t>(row * new_cols + col);
      new_cells[new_idx] = cells_[old_idx];
    }
  }

  cols_ = new_cols;
  rows_ = new_rows;
  cells_ = std::move(new_cells);
}

void GridBuffer::clear(const Cell &cell) {
  std::fill(cells_.begin(), cells_.end(), cell);
}

void GridBuffer::clear_row(int row, const Cell &cell) {
  if (row < 0 || row >= rows_) {
    return;
  }

  auto start = cells_.begin() + index(0, row);
  auto end = start + cols_;
  std::fill(start, end, cell);
}

Cell &GridBuffer::at(int col, int row) {
  if (!valid(col, row)) {
    throw std::out_of_range("GridBuffer::at: position out of range");
  }
  return cells_[index(col, row)];
}

const Cell &GridBuffer::at(int col, int row) const {
  if (!valid(col, row)) {
    throw std::out_of_range("GridBuffer::at: position out of range");
  }
  return cells_[index(col, row)];
}

void GridBuffer::set(int col, int row, const Cell &cell) {
  if (valid(col, row)) {
    cells_[index(col, row)] = cell;
  }
}

void GridBuffer::set_char(int col, int row, char32_t codepoint) {
  if (valid(col, row)) {
    cells_[index(col, row)].codepoint = codepoint;
  }
}

} // namespace tide::core
