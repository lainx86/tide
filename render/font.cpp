#include "render/font.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>

namespace tide::render {

// Static FreeType library handle
static FT_Library ft_library = nullptr;

Font::Font() = default;

Font::~Font() { shutdown(); }

bool Font::init() {
  if (ft_library) {
    return true; // Already initialized
  }

  FT_Error error = FT_Init_FreeType(&ft_library);
  if (error) {
    std::cerr << "[tide::Font] Failed to initialize FreeType: error " << error
              << std::endl;
    return false;
  }

  std::cout << "[tide::Font] FreeType initialized" << std::endl;
  return true;
}

void Font::shutdown() {
  // TODO: Free font face resources

  if (ft_library) {
    FT_Done_FreeType(ft_library);
    ft_library = nullptr;
  }

  loaded_ = false;
}

bool Font::load(const std::string &path, int size_pt) {
  if (!ft_library) {
    std::cerr << "[tide::Font] FreeType not initialized" << std::endl;
    return false;
  }

  // TODO: Actually load the font
  // FT_Face face;
  // FT_Error error = FT_New_Face(ft_library, path.c_str(), 0, &face);
  // if (error == FT_Err_Unknown_File_Format) {
  //     std::cerr << "[tide::Font] Unknown font format: " << path << std::endl;
  //     return false;
  // } else if (error) {
  //     std::cerr << "[tide::Font] Failed to load font: " << path << std::endl;
  //     return false;
  // }
  //
  // FT_Set_Char_Size(face, 0, size_pt * 64, 96, 96);
  //
  // cell_width_ = face->size->metrics.max_advance >> 6;
  // cell_height_ = face->size->metrics.height >> 6;

  std::cout << "[tide::Font] Font loading stub: " << path << " at " << size_pt
            << "pt" << std::endl;

  // Use placeholder values for now
  cell_width_ = 10;
  cell_height_ = 20;
  loaded_ = true;

  return true;
}

} // namespace tide::render
