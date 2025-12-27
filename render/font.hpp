#pragma once

#include <string>

namespace tide::render {

/**
 * Font manager for loading and rendering glyphs using FreeType.
 *
 * TODO: Implement font rendering:
 * - Load TTF/OTF fonts via FreeType
 * - Generate glyph atlas texture
 * - Handle glyph metrics for proper spacing
 * - Support font fallback chains for missing glyphs
 * - Handle DPI scaling
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
   * Load a font from file.
   * @param path Path to TTF/OTF font file
   * @param size_pt Font size in points
   * @return true if font loaded successfully
   *
   * TODO: Implement actual font loading
   */
  bool load(const std::string &path, int size_pt);

  /**
   * Get the width of a single cell in pixels.
   * For monospace fonts, this is the advance width of any glyph.
   */
  [[nodiscard]] int cell_width() const { return cell_width_; }

  /**
   * Get the height of a single cell in pixels.
   * This is typically the line height (ascent + descent + line gap).
   */
  [[nodiscard]] int cell_height() const { return cell_height_; }

  /**
   * Check if font is loaded and ready.
   */
  [[nodiscard]] bool is_loaded() const { return loaded_; }

  // TODO: Add glyph rendering methods
  // void render_glyph(char32_t codepoint, float x, float y);
  // GLuint get_atlas_texture() const;

private:
  bool loaded_ = false;
  int cell_width_ = 10;  // Placeholder default
  int cell_height_ = 20; // Placeholder default

  // TODO: Add FreeType resources
  // FT_Library ft_library_;
  // FT_Face ft_face_;
  // GLuint atlas_texture_;
  // std::unordered_map<char32_t, GlyphInfo> glyph_cache_;
};

} // namespace tide::render
