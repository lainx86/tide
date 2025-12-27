#include "render/renderer.hpp"

#include <glad/glad.h>
#include <iostream>

namespace tide::render {

Renderer::Renderer() : current_theme_(theme::get_default_theme()) {}

Renderer::~Renderer() { shutdown(); }

bool Renderer::init() {
  if (initialized_) {
    return true;
  }

  // TODO: Create shaders for text rendering
  // TODO: Create VAO/VBO for instanced glyph rendering
  // TODO: Initialize glyph atlas texture

  std::cout << "[tide::Renderer] Initialized (stub)" << std::endl;
  initialized_ = true;
  return true;
}

void Renderer::shutdown() {
  if (!initialized_) {
    return;
  }

  // TODO: Delete OpenGL resources
  // glDeleteProgram(shader_program_);
  // glDeleteVertexArrays(1, &vao_);
  // glDeleteBuffers(1, &vbo_);
  // glDeleteTextures(1, &glyph_texture_);

  initialized_ = false;
  std::cout << "[tide::Renderer] Shutdown" << std::endl;
}

void Renderer::resize(int width, int height) {
  viewport_width_ = width;
  viewport_height_ = height;
  glViewport(0, 0, width, height);

  // TODO: Recalculate terminal grid dimensions based on font metrics
}

void Renderer::render(const core::GridBuffer &grid, const theme::Theme &theme) {
  (void)grid; // TODO: Use grid for rendering

  // Clear with theme background color
  const auto &bg = theme.background;
  glClearColor(bg.r, bg.g, bg.b, bg.a);
  glClear(GL_COLOR_BUFFER_BIT);

  // TODO: Render the terminal grid
  // 1. Bind shader program
  // 2. Bind glyph atlas texture
  // 3. For each cell in the visible grid:
  //    - Draw background quad if color differs from default
  //    - Draw glyph from atlas
  // 4. Draw cursor
  // 5. Draw selection overlay
}

void Renderer::set_theme(const theme::Theme &theme) { current_theme_ = theme; }

} // namespace tide::render
