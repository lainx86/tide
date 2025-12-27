#include "core/ansi_parser.hpp"

#include <iomanip>
#include <iostream>


namespace tide::core {

void AnsiParser::feed(const char *data, size_t size) {
  // TODO: Implement proper ANSI escape sequence parsing
  // For now, just log the raw data for debugging

  std::cout << "[tide::AnsiParser] Received " << size << " bytes: ";

  for (size_t i = 0; i < size && i < 64; ++i) {
    unsigned char c = static_cast<unsigned char>(data[i]);
    if (c >= 32 && c < 127) {
      std::cout << static_cast<char>(c);
    } else if (c == '\n') {
      std::cout << "\\n";
    } else if (c == '\r') {
      std::cout << "\\r";
    } else if (c == '\t') {
      std::cout << "\\t";
    } else if (c == 0x1B) {
      std::cout << "\\e";
    } else {
      std::cout << "\\x" << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(c) << std::dec;
    }
  }

  if (size > 64) {
    std::cout << "... (" << (size - 64) << " more bytes)";
  }

  std::cout << std::endl;
}

void AnsiParser::reset() {
  buffer_.clear();
  // TODO: Reset state machine to initial state
}

} // namespace tide::core
