#pragma once

#include "core/grid_buffer.hpp"
#include "theme/theme.hpp"


namespace tide::render {

/**
 * OpenGL renderer for the terminal.
 *
 * TODO: Implement full rendering pipeline:
 * - Glyph atlas generation from font
 * - Instanced rendering for character grid
 * - Selection highlighting
 * - Cursor rendering with blink animation
 * - Underline/strikethrough decorations
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
   * @return true if initialization succeeded
   */
  bool init();

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
   * Currently just clears the screen with the theme background color.
   *
   * @param grid The terminal grid to render
   * @param theme The color theme to use
   *
   * TODO: Implement actual grid rendering
   */
  void render(const core::GridBuffer &grid, const theme::Theme &theme);

  /**
   * Set the current theme.
   */
  void set_theme(const theme::Theme &theme);

private:
  int viewport_width_ = 0;
  int viewport_height_ = 0;
  theme::Theme current_theme_;
  bool initialized_ = false;

  // TODO: Add OpenGL resources
  // GLuint shader_program_;
  // GLuint vao_, vbo_, ebo_;
  // GLuint glyph_texture_;
};

} // namespace tide::render
