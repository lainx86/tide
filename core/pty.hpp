#pragma once

#include <string>

namespace tide::core {

/**
 * PTY (Pseudo-Terminal) handler for spawning and communicating with a shell.
 * Uses Linux forkpty() to create a pseudo-terminal pair.
 */
class Pty {
public:
  Pty();
  ~Pty();

  // Non-copyable
  Pty(const Pty &) = delete;
  Pty &operator=(const Pty &) = delete;

  // Movable
  Pty(Pty &&other) noexcept;
  Pty &operator=(Pty &&other) noexcept;

  /**
   * Spawn a shell process attached to the PTY.
   * Uses $SHELL environment variable, falls back to /bin/bash.
   * @return true if spawn succeeded
   */
  bool spawn();

  /**
   * Read available data from the PTY (non-blocking).
   * @param buffer Output buffer to read into
   * @param max_size Maximum bytes to read
   * @return Number of bytes read, 0 if nothing available, -1 on error
   */
  ssize_t read(char *buffer, size_t max_size);

  /**
   * Write data to the PTY (sends to shell's stdin).
   * @param data Data to write
   * @param size Size of data
   * @return Number of bytes written, -1 on error
   */
  ssize_t write(const char *data, size_t size);

  /**
   * Resize the PTY window size.
   * @param cols Number of columns
   * @param rows Number of rows
   */
  void resize(int cols, int rows);

  /**
   * Close the PTY and terminate the child process.
   */
  void close();

  /**
   * Check if the PTY is currently open and valid.
   */
  [[nodiscard]] bool is_open() const { return master_fd_ >= 0; }

  /**
   * Get the master file descriptor (for advanced use).
   */
  [[nodiscard]] int master_fd() const { return master_fd_; }

private:
  int master_fd_ = -1;   // Master side of the PTY
  pid_t child_pid_ = -1; // Child process ID
};

} // namespace tide::core
