#include "core/pty.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>


namespace tide::core {

Pty::Pty() = default;

Pty::~Pty() { close(); }

Pty::Pty(Pty &&other) noexcept
    : master_fd_(other.master_fd_), child_pid_(other.child_pid_) {
  other.master_fd_ = -1;
  other.child_pid_ = -1;
}

Pty &Pty::operator=(Pty &&other) noexcept {
  if (this != &other) {
    close();
    master_fd_ = other.master_fd_;
    child_pid_ = other.child_pid_;
    other.master_fd_ = -1;
    other.child_pid_ = -1;
  }
  return *this;
}

bool Pty::spawn() {
  // Get shell from environment, fallback to /bin/bash
  const char *shell = std::getenv("SHELL");
  if (!shell || shell[0] == '\0') {
    shell = "/bin/bash";
  }

  // Create PTY and fork
  pid_t pid = forkpty(&master_fd_, nullptr, nullptr, nullptr);

  if (pid < 0) {
    std::cerr << "[tide] forkpty() failed: " << std::strerror(errno)
              << std::endl;
    return false;
  }

  if (pid == 0) {
    // Child process - exec the shell
    // TODO: Set up environment variables for terminal type
    setenv("TERM", "xterm-256color", 1);

    // Execute shell as login shell
    execlp(shell, shell, "-l", nullptr);

    // If exec fails, exit child
    std::cerr << "[tide] exec failed: " << std::strerror(errno) << std::endl;
    _exit(1);
  }

  // Parent process
  child_pid_ = pid;
  std::cout << "[tide] Spawned shell: " << shell << " (pid: " << pid << ")"
            << std::endl;

  return true;
}

ssize_t Pty::read(char *buffer, size_t max_size) {
  if (master_fd_ < 0) {
    return -1;
  }

  // Use select() for non-blocking check
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(master_fd_, &read_fds);

  struct timeval timeout = {0, 0}; // Immediate return

  int result = select(master_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);

  if (result < 0) {
    if (errno == EINTR) {
      return 0; // Interrupted, try again later
    }
    return -1;
  }

  if (result == 0) {
    return 0; // No data available
  }

  // Data available, read it
  ssize_t bytes_read = ::read(master_fd_, buffer, max_size);

  if (bytes_read < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    return -1;
  }

  return bytes_read;
}

ssize_t Pty::write(const char *data, size_t size) {
  if (master_fd_ < 0) {
    return -1;
  }

  return ::write(master_fd_, data, size);
}

void Pty::resize(int cols, int rows) {
  if (master_fd_ < 0) {
    return;
  }

  struct winsize ws = {};
  ws.ws_col = static_cast<unsigned short>(cols);
  ws.ws_row = static_cast<unsigned short>(rows);

  // TODO: Calculate pixel dimensions based on font metrics
  ws.ws_xpixel = 0;
  ws.ws_ypixel = 0;

  if (ioctl(master_fd_, TIOCSWINSZ, &ws) < 0) {
    std::cerr << "[tide] Failed to resize PTY: " << std::strerror(errno)
              << std::endl;
  }
}

void Pty::close() {
  if (master_fd_ >= 0) {
    ::close(master_fd_);
    master_fd_ = -1;
  }

  if (child_pid_ > 0) {
    // Wait for child to avoid zombie process
    int status;
    waitpid(child_pid_, &status, WNOHANG);
    child_pid_ = -1;
  }
}

} // namespace tide::core
