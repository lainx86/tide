#pragma once

#include <string>
#include <string_view>

namespace tide::core {

/**
 * ANSI escape sequence parser (stub implementation).
 *
 * TODO: Implement full ANSI/VT100 escape sequence parsing:
 * - CSI sequences (cursor movement, attributes, etc.)
 * - OSC sequences (window title, colors, etc.)
 * - DCS sequences (device control)
 * - SGR attributes (colors, bold, italic, etc.)
 */
class AnsiParser {
public:
  AnsiParser() = default;

  /**
   * Feed raw data from PTY into the parser.
   * Currently just logs the data for debugging.
   *
   * @param data Raw bytes from PTY
   * @param size Number of bytes
   *
   * TODO: Parse escape sequences and emit events/callbacks
   * TODO: Buffer incomplete sequences across calls
   */
  void feed(const char *data, size_t size);

  /**
   * Reset parser state.
   *
   * TODO: Clear any buffered partial sequences
   */
  void reset();

private:
  // TODO: Add parser state machine
  // TODO: Add callback/event system for parsed sequences
  // TODO: Add buffer for incomplete sequences

  std::string buffer_; // Temporary buffer for incomplete sequences
};

} // namespace tide::core
