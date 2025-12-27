#include "render/renderer.hpp"

#include <glad/glad.h>

#include <cstring>
#include <functional>
#include <iostream>
#include <vector>

namespace tide::render {

// Vertex shader source
static const char *VERTEX_SHADER_SOURCE = R"(
#version 330 core

// Per-vertex attributes (quad)
layout (location = 0) in vec2 a_vertex;

// Per-instance attributes
layout (location = 1) in vec2 a_cell_pos;
layout (location = 2) in vec4 a_tex_coords;   // x0, y0, x1, y1
layout (location = 3) in vec2 a_glyph_offset;
layout (location = 4) in vec2 a_glyph_size;
layout (location = 5) in vec4 a_fg_color;
layout (location = 6) in vec4 a_bg_color;

uniform mat4 u_projection;
uniform vec2 u_cell_size;

out vec2 v_tex_coord;
out vec4 v_fg_color;
out vec4 v_bg_color;
out float v_is_background;

void main() {
    // Determine if this is background quad (vertex.x < 0.5) or glyph quad
    v_is_background = step(a_vertex.x, 0.25);
    
    vec2 pos;
    if (v_is_background > 0.5) {
        // Background quad: fill entire cell
        pos = a_cell_pos + a_vertex * u_cell_size;
        v_tex_coord = vec2(0.0);
    } else {
        // Glyph quad: offset within cell
        vec2 local_vert = a_vertex - vec2(0.5, 0.0);  // Shift glyph verts
        pos = a_cell_pos + a_glyph_offset + local_vert * a_glyph_size;
        
        // Interpolate texture coordinates
        v_tex_coord = mix(a_tex_coords.xy, a_tex_coords.zw, local_vert);
    }
    
    gl_Position = u_projection * vec4(pos, 0.0, 1.0);
    v_fg_color = a_fg_color;
    v_bg_color = a_bg_color;
}
)";

// Fragment shader source
static const char *FRAGMENT_SHADER_SOURCE = R"(
#version 330 core

in vec2 v_tex_coord;
in vec4 v_fg_color;
in vec4 v_bg_color;
in float v_is_background;

uniform sampler2D u_atlas;

out vec4 frag_color;

void main() {
    if (v_is_background > 0.5) {
        // Background: solid color
        frag_color = v_bg_color;
    } else {
        // Glyph: sample atlas and apply foreground color
        float alpha = texture(u_atlas, v_tex_coord).r;
        frag_color = vec4(v_fg_color.rgb, v_fg_color.a * alpha);
    }
}
)";

Renderer::Renderer() : current_theme_(theme::get_default_theme()) {}

Renderer::~Renderer() { shutdown(); }

bool Renderer::init(Font *font) {
  if (initialized_) {
    return true;
  }

  font_ = font;
  if (!font_ || !font_->is_loaded()) {
    std::cerr << "[tide::Renderer] Font not loaded" << std::endl;
    return false;
  }

  // Create shaders
  if (!create_shaders()) {
    return false;
  }

  // Create VAO
  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

  // Create quad VBO (two triangles: background + glyph)
  // Background uses verts 0-1, Glyph uses verts 0.5-1.5
  float quad_vertices[] = {
      // Background quad (0,0 to 1,1)
      0.0f,
      0.0f,
      1.0f,
      0.0f,
      1.0f,
      1.0f,
      0.0f,
      0.0f,
      1.0f,
      1.0f,
      0.0f,
      1.0f,
      // Glyph quad (0.5,0 to 1.5,1)
      0.5f,
      0.0f,
      1.5f,
      0.0f,
      1.5f,
      1.0f,
      0.5f,
      0.0f,
      1.5f,
      1.0f,
      0.5f,
      1.0f,
  };

  glGenBuffers(1, &vbo_quad_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_quad_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices,
               GL_STATIC_DRAW);

  // Vertex attribute (location 0)
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

  // Create instance VBO (will be filled each frame)
  glGenBuffers(1, &vbo_instances_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_instances_);
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

  // Instance attributes
  size_t stride = sizeof(CellInstance);

  // a_cell_pos (location 1)
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(CellInstance, pos_x));
  glVertexAttribDivisor(1, 1);

  // a_tex_coords (location 2)
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(CellInstance, tex_x0));
  glVertexAttribDivisor(2, 1);

  // a_glyph_offset (location 3)
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(CellInstance, glyph_offset_x));
  glVertexAttribDivisor(3, 1);

  // a_glyph_size (location 4)
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(CellInstance, glyph_size_x));
  glVertexAttribDivisor(4, 1);

  // a_fg_color (location 5)
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(CellInstance, fg_r));
  glVertexAttribDivisor(5, 1);

  // a_bg_color (location 6)
  glEnableVertexAttribArray(6);
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(CellInstance, bg_r));
  glVertexAttribDivisor(6, 1);

  glBindVertexArray(0);

  // Get uniform locations
  loc_projection_ = glGetUniformLocation(shader_program_, "u_projection");
  loc_cell_size_ = glGetUniformLocation(shader_program_, "u_cell_size");
  loc_atlas_texture_ = glGetUniformLocation(shader_program_, "u_atlas");

  // Enable blending for text
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  initialized_ = true;
  std::cout << "[tide::Renderer] Initialized" << std::endl;
  return true;
}

bool Renderer::create_shaders() {
  // Compile vertex shader
  GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vert_shader, 1, &VERTEX_SHADER_SOURCE, nullptr);
  glCompileShader(vert_shader);

  GLint success;
  glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char log[512];
    glGetShaderInfoLog(vert_shader, 512, nullptr, log);
    std::cerr << "[tide::Renderer] Vertex shader error: " << log << std::endl;
    return false;
  }

  // Compile fragment shader
  GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag_shader, 1, &FRAGMENT_SHADER_SOURCE, nullptr);
  glCompileShader(frag_shader);

  glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char log[512];
    glGetShaderInfoLog(frag_shader, 512, nullptr, log);
    std::cerr << "[tide::Renderer] Fragment shader error: " << log << std::endl;
    return false;
  }

  // Link program
  shader_program_ = glCreateProgram();
  glAttachShader(shader_program_, vert_shader);
  glAttachShader(shader_program_, frag_shader);
  glLinkProgram(shader_program_);

  glGetProgramiv(shader_program_, GL_LINK_STATUS, &success);
  if (!success) {
    char log[512];
    glGetProgramInfoLog(shader_program_, 512, nullptr, log);
    std::cerr << "[tide::Renderer] Shader link error: " << log << std::endl;
    return false;
  }

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);

  return true;
}

void Renderer::shutdown() {
  if (vbo_instances_) {
    glDeleteBuffers(1, &vbo_instances_);
    vbo_instances_ = 0;
  }
  if (vbo_quad_) {
    glDeleteBuffers(1, &vbo_quad_);
    vbo_quad_ = 0;
  }
  if (vao_) {
    glDeleteVertexArrays(1, &vao_);
    vao_ = 0;
  }
  if (shader_program_) {
    glDeleteProgram(shader_program_);
    shader_program_ = 0;
  }

  initialized_ = false;
}

void Renderer::resize(int width, int height) {
  viewport_width_ = width;
  viewport_height_ = height;
  glViewport(0, 0, width, height);
}

int Renderer::grid_cols() const {
  if (!font_ || font_->cell_width() == 0)
    return 80;
  return viewport_width_ / font_->cell_width();
}

int Renderer::grid_rows() const {
  if (!font_ || font_->cell_height() == 0)
    return 24;
  return viewport_height_ / font_->cell_height();
}

void Renderer::render(const core::GridBuffer &grid, const theme::Theme &theme,
                      int cursor_col, int cursor_row, bool show_cursor) {
  if (!initialized_ || !font_) {
    return;
  }

  // Clear with background color
  const auto &bg = theme.background;
  glClearColor(bg.r, bg.g, bg.b, bg.a);
  glClear(GL_COLOR_BUFFER_BIT);

  // Use shader
  glUseProgram(shader_program_);

  // Set orthographic projection (flip Y for top-left origin)
  float proj[16] = {2.0f / viewport_width_,
                    0.0f,
                    0.0f,
                    0.0f,
                    0.0f,
                    -2.0f / viewport_height_,
                    0.0f,
                    0.0f,
                    0.0f,
                    0.0f,
                    -1.0f,
                    0.0f,
                    -1.0f,
                    1.0f,
                    0.0f,
                    1.0f};
  glUniformMatrix4fv(loc_projection_, 1, GL_FALSE, proj);

  // Set cell size
  glUniform2f(loc_cell_size_, static_cast<float>(font_->cell_width()),
              static_cast<float>(font_->cell_height()));

  // Bind atlas texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, font_->atlas_texture());
  glUniform1i(loc_atlas_texture_, 0);

  // Build instance data
  update_instances(grid, theme, cursor_col, cursor_row, show_cursor);

  // Draw all cells
  glBindVertexArray(vao_);

  int num_cells = grid.cols() * grid.rows();

  // Draw backgrounds first (6 vertices per cell, using first 6 verts of quad)
  glDrawArraysInstanced(GL_TRIANGLES, 0, 6, num_cells);

  // Draw glyphs (6 vertices per cell, using last 6 verts of quad)
  glDrawArraysInstanced(GL_TRIANGLES, 6, 6, num_cells);

  glBindVertexArray(0);
}

void Renderer::update_instances(const core::GridBuffer &grid,
                                const theme::Theme &theme, int cursor_col,
                                int cursor_row, bool show_cursor) {
  int cols = grid.cols();
  int rows = grid.rows();
  int cell_w = font_->cell_width();
  int cell_h = font_->cell_height();
  int baseline = font_->baseline();

  std::vector<CellInstance> instances;
  instances.reserve(cols * rows);

  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      const core::Cell &cell = grid.at(col, row);

      CellInstance inst;
      inst.pos_x = static_cast<float>(col * cell_w);
      inst.pos_y = static_cast<float>(row * cell_h);

      // Get glyph info
      const GlyphInfo &glyph = font_->get_glyph(cell.codepoint);
      inst.tex_x0 = glyph.tex_x0;
      inst.tex_y0 = glyph.tex_y0;
      inst.tex_x1 = glyph.tex_x1;
      inst.tex_y1 = glyph.tex_y1;

      // Glyph positioning within cell
      inst.glyph_offset_x = static_cast<float>(glyph.bearing_x);
      inst.glyph_offset_y = static_cast<float>(baseline - glyph.bearing_y);
      inst.glyph_size_x = static_cast<float>(glyph.width);
      inst.glyph_size_y = static_cast<float>(glyph.height);

      // Colors
      theme::Color fg = cell.foreground;
      theme::Color bg = cell.background;

      // Selection: invert colors (check before cursor)
      if (selection_check_ && selection_check_(col, row)) {
        std::swap(fg, bg);
      }
      // Cursor: invert colors
      else if (show_cursor && col == cursor_col && row == cursor_row) {
        std::swap(fg, bg);
      }

      inst.fg_r = fg.r;
      inst.fg_g = fg.g;
      inst.fg_b = fg.b;
      inst.fg_a = fg.a;
      inst.bg_r = bg.r;
      inst.bg_g = bg.g;
      inst.bg_b = bg.b;
      inst.bg_a = bg.a;

      instances.push_back(inst);
    }
  }

  // Upload instance data
  glBindBuffer(GL_ARRAY_BUFFER, vbo_instances_);
  glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(CellInstance),
               instances.data(), GL_DYNAMIC_DRAW);
}

void Renderer::set_theme(const theme::Theme &theme) { current_theme_ = theme; }

} // namespace tide::render
