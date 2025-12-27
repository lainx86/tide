#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace tide::theme {

/**
 * RGBA color representation using normalized floats (0.0 - 1.0).
 * Suitable for OpenGL.
 */
struct Color {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 1.0f;

  constexpr Color() = default;
  constexpr Color(float r, float g, float b, float a = 1.0f)
      : r(r), g(g), b(b), a(a) {}

  /**
   * Create color from hex value (e.g., 0xRRGGBB).
   */
  static constexpr Color from_hex(uint32_t hex, float alpha = 1.0f) {
    return Color{static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
                 static_cast<float>((hex >> 8) & 0xFF) / 255.0f,
                 static_cast<float>(hex & 0xFF) / 255.0f, alpha};
  }
};

/**
 * Terminal color theme.
 * Contains the 16 standard ANSI colors plus UI colors.
 */
struct Theme {
  std::string name;

  // Standard ANSI colors (0-15)
  // 0-7: Normal colors (black, red, green, yellow, blue, magenta, cyan, white)
  // 8-15: Bright variants
  std::array<Color, 16> ansi_colors;

  // UI colors
  Color foreground; // Default text color
  Color background; // Window background
  Color cursor;     // Cursor color
  Color selection;  // Selection highlight

  // TODO: Add more theme colors as needed:
  // - Bold text color
  // - Dim text color
  // - Border colors
  // - Search highlight
};

/**
 * Get the built-in Tokyo Night theme.
 */
Theme get_tokyo_night_theme();

/**
 * Get the built-in Dracula theme.
 */
Theme get_dracula_theme();

/**
 * Get the default theme.
 */
inline Theme get_default_theme() { return get_tokyo_night_theme(); }

// TODO: Add functions for loading themes from files
// Theme load_theme_from_file(const std::string& path);
// Theme load_theme_from_json(const std::string& json);

} // namespace tide::theme
