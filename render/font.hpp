#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

// Forward declarations for FreeType
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_FaceRec_ *FT_Face;

namespace tide::render {

/**
 * Information about a single glyph in the atlas.
 */
struct GlyphInfo {
  float tex_x0, tex_y0; // Top-left texture coordinates (normalized)
  float tex_x1, tex_y1; // Bottom-right texture coordinates (normalized)
  int width, height;    // Glyph bitmap dimensions
  int bearing_x;        // Offset from cursor to left edge
  int bearing_y;        // Offset from baseline to top edge
  int advance;          // Horizontal advance to next glyph
};

/**
 * Font manager for loading and rendering glyphs using FreeType.
 * Generates a glyph atlas texture for efficient OpenGL rendering.
 */
class Font {
public:
  Font();
  ~Font();

  // Non-copyable
  Font(const Font &) = delete;
  Font &operator=(const Font &) = delete;

  /**
   * Initialize FreeType library.
   * @return true if initialization succeeded
   */
  bool init();

  /**
   * Clean up FreeType resources.
   */
  void shutdown();

  /**
   * Load a font from file and generate glyph atlas.
   * @param path Path to TTF/OTF font file
   * @param size_px Font size in pixels
   * @return true if font loaded successfully
   */
  bool load(const std::string &path, int size_px);

  /**
   * Get glyph info for a codepoint.
   * Returns info for '?' if glyph not found.
   */
  [[nodiscard]] const GlyphInfo &get_glyph(char32_t codepoint) const;

  /**
   * Get the OpenGL texture ID of the glyph atlas.
   */
  [[nodiscard]] uint32_t atlas_texture() const { return atlas_texture_; }

  /**
   * Get atlas dimensions.
   */
  [[nodiscard]] int atlas_width() const { return atlas_width_; }
  [[nodiscard]] int atlas_height() const { return atlas_height_; }

  /**
   * Get the width of a single cell in pixels.
   * For monospace fonts, this is the advance width of any glyph.
   */
  [[nodiscard]] int cell_width() const { return cell_width_; }

  /**
   * Get the height of a single cell in pixels.
   */
  [[nodiscard]] int cell_height() const { return cell_height_; }

  /**
   * Get baseline offset from top of cell.
   */
  [[nodiscard]] int baseline() const { return baseline_; }

  /**
   * Check if font is loaded and ready.
   */
  [[nodiscard]] bool is_loaded() const { return loaded_; }

private:
  bool loaded_ = false;

  // FreeType handles
  FT_Library ft_library_ = nullptr;
  FT_Face ft_face_ = nullptr;

  // Atlas texture
  uint32_t atlas_texture_ = 0;
  int atlas_width_ = 0;
  int atlas_height_ = 0;

  // Font metrics
  int cell_width_ = 0;
  int cell_height_ = 0;
  int baseline_ = 0;
  int ascent_ = 0;
  int descent_ = 0;

  // Glyph cache
  std::unordered_map<char32_t, GlyphInfo> glyphs_;
  GlyphInfo fallback_glyph_;

  /**
   * Generate glyph atlas from loaded font face.
   */
  bool generate_atlas();
};

} // namespace tide::render
