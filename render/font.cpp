#include "render/font.hpp"

#include <ft2build.h>
#include <glad/glad.h>

#include FT_FREETYPE_H

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

namespace tide::render {

Font::Font() = default;

Font::~Font() { shutdown(); }

bool Font::init() {
  if (ft_library_) {
    return true;
  }

  FT_Error error = FT_Init_FreeType(&ft_library_);
  if (error) {
    std::cerr << "[tide::Font] Failed to initialize FreeType: " << error
              << std::endl;
    return false;
  }

  std::cout << "[tide::Font] FreeType initialized" << std::endl;
  return true;
}

void Font::shutdown() {
  if (atlas_texture_) {
    glDeleteTextures(1, &atlas_texture_);
    atlas_texture_ = 0;
  }

  if (ft_face_) {
    FT_Done_Face(ft_face_);
    ft_face_ = nullptr;
  }

  if (ft_library_) {
    FT_Done_FreeType(ft_library_);
    ft_library_ = nullptr;
  }

  glyphs_.clear();
  loaded_ = false;
}

bool Font::load(const std::string &path, int size_px) {
  if (!ft_library_) {
    std::cerr << "[tide::Font] FreeType not initialized" << std::endl;
    return false;
  }

  // Clean up previous font if any
  if (ft_face_) {
    FT_Done_Face(ft_face_);
    ft_face_ = nullptr;
  }
  if (atlas_texture_) {
    glDeleteTextures(1, &atlas_texture_);
    atlas_texture_ = 0;
  }
  glyphs_.clear();

  // Load font face
  FT_Error error = FT_New_Face(ft_library_, path.c_str(), 0, &ft_face_);
  if (error == FT_Err_Unknown_File_Format) {
    std::cerr << "[tide::Font] Unknown font format: " << path << std::endl;
    return false;
  } else if (error) {
    std::cerr << "[tide::Font] Failed to load font: " << path << std::endl;
    return false;
  }

  // Set pixel size
  error = FT_Set_Pixel_Sizes(ft_face_, 0, size_px);
  if (error) {
    std::cerr << "[tide::Font] Failed to set font size" << std::endl;
    return false;
  }

  // Calculate font metrics
  ascent_ = ft_face_->size->metrics.ascender >> 6;
  descent_ = -(ft_face_->size->metrics.descender >> 6);
  cell_height_ = ascent_ + descent_;
  baseline_ = ascent_;

  // For monospace fonts, get the advance width of any character
  FT_Load_Char(ft_face_, 'M', FT_LOAD_DEFAULT);
  cell_width_ = ft_face_->glyph->advance.x >> 6;

  std::cout << "[tide::Font] Loaded: " << path << std::endl;
  std::cout << "[tide::Font] Cell: " << cell_width_ << "x" << cell_height_
            << " (baseline: " << baseline_ << ")" << std::endl;

  // Generate the glyph atlas
  if (!generate_atlas()) {
    return false;
  }

  loaded_ = true;
  return true;
}

bool Font::generate_atlas() {
  // Characters to include in atlas (ASCII printable + some extended)
  const int first_char = 32;
  const int last_char = 126;
  const int num_chars = last_char - first_char + 1;

  // Calculate atlas dimensions
  // Arrange glyphs in a grid
  int chars_per_row = 16;
  int num_rows = (num_chars + chars_per_row - 1) / chars_per_row;

  // Add padding for each glyph
  int glyph_padding = 1;
  atlas_width_ = chars_per_row * (cell_width_ + glyph_padding);
  atlas_height_ = num_rows * (cell_height_ + glyph_padding);

  // Make atlas dimensions power of 2 (optional but good practice)
  auto next_pow2 = [](int v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
  };
  atlas_width_ = next_pow2(atlas_width_);
  atlas_height_ = next_pow2(atlas_height_);

  std::cout << "[tide::Font] Atlas size: " << atlas_width_ << "x"
            << atlas_height_ << std::endl;

  // Create bitmap buffer (single channel)
  std::vector<uint8_t> atlas_data(atlas_width_ * atlas_height_, 0);

  // Render each glyph to atlas
  int pen_x = glyph_padding;
  int pen_y = glyph_padding;

  for (int c = first_char; c <= last_char; ++c) {
    // Load and render glyph
    FT_Error error = FT_Load_Char(ft_face_, c, FT_LOAD_RENDER);
    if (error) {
      continue;
    }

    FT_GlyphSlot g = ft_face_->glyph;

    // Check if we need to wrap to next row
    if (pen_x + cell_width_ + glyph_padding > atlas_width_) {
      pen_x = glyph_padding;
      pen_y += cell_height_ + glyph_padding;
    }

    // Copy glyph bitmap to atlas
    for (unsigned int row = 0; row < g->bitmap.rows; ++row) {
      for (unsigned int col = 0; col < g->bitmap.width; ++col) {
        int atlas_x = pen_x + col;
        int atlas_y = pen_y + row;
        if (atlas_x < atlas_width_ && atlas_y < atlas_height_) {
          atlas_data[atlas_y * atlas_width_ + atlas_x] =
              g->bitmap.buffer[row * g->bitmap.width + col];
        }
      }
    }

    // Store glyph info
    GlyphInfo info;
    info.tex_x0 = static_cast<float>(pen_x) / atlas_width_;
    info.tex_y0 = static_cast<float>(pen_y) / atlas_height_;
    info.tex_x1 = static_cast<float>(pen_x + g->bitmap.width) / atlas_width_;
    info.tex_y1 = static_cast<float>(pen_y + g->bitmap.rows) / atlas_height_;
    info.width = g->bitmap.width;
    info.height = g->bitmap.rows;
    info.bearing_x = g->bitmap_left;
    info.bearing_y = g->bitmap_top;
    info.advance = g->advance.x >> 6;

    glyphs_[static_cast<char32_t>(c)] = info;

    // Move pen to next position
    pen_x += cell_width_ + glyph_padding;
  }

  // Store fallback glyph ('?')
  auto it = glyphs_.find(U'?');
  if (it != glyphs_.end()) {
    fallback_glyph_ = it->second;
  }

  // Create OpenGL texture
  glGenTextures(1, &atlas_texture_);
  glBindTexture(GL_TEXTURE_2D, atlas_texture_);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Upload texture data (single channel = GL_RED)
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas_width_, atlas_height_, 0, GL_RED,
               GL_UNSIGNED_BYTE, atlas_data.data());

  glBindTexture(GL_TEXTURE_2D, 0);

  std::cout << "[tide::Font] Generated atlas with " << glyphs_.size()
            << " glyphs" << std::endl;
  return true;
}

const GlyphInfo &Font::get_glyph(char32_t codepoint) const {
  auto it = glyphs_.find(codepoint);
  if (it != glyphs_.end()) {
    return it->second;
  }
  return fallback_glyph_;
}

} // namespace tide::render
