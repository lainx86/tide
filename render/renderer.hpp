#pragma once

#include "core/grid_buffer.hpp"
#include "render/font.hpp"
#include "theme/theme.hpp"

#include <cstdint>
#include <functional>

namespace tide::render {

/**
 * OpenGL renderer for the terminal.
 * Uses instanced rendering to draw the character grid efficiently.
 */
class Renderer {
public:
  Renderer();
  ~Renderer();

  // Non-copyable
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  /**
   * Initialize OpenGL resources.
   * Must be called after OpenGL context is created.
   * @param font The font to use for rendering
   * @return true if initialization succeeded
   */
  bool init(Font *font);

  /**
   * Clean up OpenGL resources.
   */
  void shutdown();

  /**
   * Update viewport dimensions.
   * @param width Viewport width in pixels
   * @param height Viewport height in pixels
   */
  void resize(int width, int height);

  /**
   * Render a frame.
   * @param grid The terminal grid to render
   * @param theme The color theme to use
   * @param cursor_col Cursor column position
   * @param cursor_row Cursor row position
   * @param show_cursor Whether to show cursor
   */
  void render(const core::GridBuffer &grid, const theme::Theme &theme,
              int cursor_col = -1, int cursor_row = -1,
              bool show_cursor = true);

  /**
   * Set the current theme.
   */
  void set_theme(const theme::Theme &theme);

  /**
   * Get calculated grid dimensions based on viewport and font.
   */
  [[nodiscard]] int grid_cols() const;
  [[nodiscard]] int grid_rows() const;

  // Selection callback type
  using SelectionCheck = std::function<bool(int col, int row)>;

  /**
   * Set selection check function for highlighting.
   */
  void set_selection_check(SelectionCheck check) { selection_check_ = check; }

private:
  Font *font_ = nullptr;
  int viewport_width_ = 0;
  int viewport_height_ = 0;
  theme::Theme current_theme_;
  bool initialized_ = false;
  SelectionCheck selection_check_ = nullptr;

  // OpenGL resources
  uint32_t shader_program_ = 0;
  uint32_t vao_ = 0;
  uint32_t vbo_quad_ = 0;
  uint32_t vbo_instances_ = 0;

  // Uniform locations
  int loc_projection_ = -1;
  int loc_cell_size_ = -1;
  int loc_atlas_texture_ = -1;

  // Instance data for batch rendering
  struct CellInstance {
    float pos_x, pos_y;   // Cell position in pixels
    float tex_x0, tex_y0; // Glyph texture coords
    float tex_x1, tex_y1;
    float glyph_offset_x; // Offset within cell
    float glyph_offset_y;
    float glyph_size_x; // Glyph size in pixels
    float glyph_size_y;
    float fg_r, fg_g, fg_b, fg_a; // Foreground color
    float bg_r, bg_g, bg_b, bg_a; // Background color
  };

  bool create_shaders();
  void update_instances(const core::GridBuffer &grid, const theme::Theme &theme,
                        int cursor_col, int cursor_row, bool show_cursor);
};

} // namespace tide::render
