#include "theme/theme.hpp"

namespace tide::theme {

Theme get_tokyo_night_theme() {
  Theme theme;
  theme.name = "Tokyo Night";

  // ANSI colors based on Tokyo Night color palette
  theme.ansi_colors = {{
      // Normal colors (0-7)
      Color::from_hex(0x15161e), // 0: Black
      Color::from_hex(0xf7768e), // 1: Red
      Color::from_hex(0x9ece6a), // 2: Green
      Color::from_hex(0xe0af68), // 3: Yellow
      Color::from_hex(0x7aa2f7), // 4: Blue
      Color::from_hex(0xbb9af7), // 5: Magenta
      Color::from_hex(0x7dcfff), // 6: Cyan
      Color::from_hex(0xa9b1d6), // 7: White

      // Bright colors (8-15)
      Color::from_hex(0x414868), // 8: Bright Black
      Color::from_hex(0xf7768e), // 9: Bright Red
      Color::from_hex(0x9ece6a), // 10: Bright Green
      Color::from_hex(0xe0af68), // 11: Bright Yellow
      Color::from_hex(0x7aa2f7), // 12: Bright Blue
      Color::from_hex(0xbb9af7), // 13: Bright Magenta
      Color::from_hex(0x7dcfff), // 14: Bright Cyan
      Color::from_hex(0xc0caf5), // 15: Bright White
  }};

  // UI colors
  theme.foreground = Color::from_hex(0xc0caf5);
  theme.background = Color::from_hex(0x1a1b26);
  theme.cursor = Color::from_hex(0xc0caf5);
  theme.selection = Color::from_hex(0x33467c);

  return theme;
}

Theme get_dracula_theme() {
  Theme theme;
  theme.name = "Dracula";

  // ANSI colors based on Dracula color palette
  theme.ansi_colors = {{
      // Normal colors (0-7)
      Color::from_hex(0x21222c), // 0: Black
      Color::from_hex(0xff5555), // 1: Red
      Color::from_hex(0x50fa7b), // 2: Green
      Color::from_hex(0xf1fa8c), // 3: Yellow
      Color::from_hex(0xbd93f9), // 4: Blue
      Color::from_hex(0xff79c6), // 5: Magenta
      Color::from_hex(0x8be9fd), // 6: Cyan
      Color::from_hex(0xf8f8f2), // 7: White

      // Bright colors (8-15)
      Color::from_hex(0x6272a4), // 8: Bright Black
      Color::from_hex(0xff6e6e), // 9: Bright Red
      Color::from_hex(0x69ff94), // 10: Bright Green
      Color::from_hex(0xffffa5), // 11: Bright Yellow
      Color::from_hex(0xd6acff), // 12: Bright Blue
      Color::from_hex(0xff92df), // 13: Bright Magenta
      Color::from_hex(0xa4ffff), // 14: Bright Cyan
      Color::from_hex(0xffffff), // 15: Bright White
  }};

  // UI colors
  theme.foreground = Color::from_hex(0xf8f8f2);
  theme.background = Color::from_hex(0x282a36);
  theme.cursor = Color::from_hex(0xf8f8f2);
  theme.selection = Color::from_hex(0x44475a);

  return theme;
}

} // namespace tide::theme
