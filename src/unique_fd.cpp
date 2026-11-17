#include "miniposix/unique_fd.hpp"

#include <unistd.h>

#include <utility>

namespace miniposix {

UniqueFd::UniqueFd(int fd) noexcept : fd_(fd) {}

UniqueFd::~UniqueFd() noexcept {
    reset();
}

UniqueFd::UniqueFd(UniqueFd&& other) noexcept : fd_(other.release()) {}

UniqueFd& UniqueFd::operator=(UniqueFd&& other) noexcept {
    // reset() closes our old descriptor before adopting the new one. The
    // self-assignment guard keeps `x = std::move(x)` from closing x's own fd.
    if(this != &other) {
        reset(other.release());
    }
    return *this;
}

int UniqueFd::get() const noexcept {
    return fd_;
}

int UniqueFd::release() noexcept {
    return std::exchange(fd_, -1);
}

void UniqueFd::reset(int fd) noexcept {
    if(fd == fd_) {
        return; // reset(get()) must not close the descriptor we keep owning.
    }
    const int old = std::exchange(fd_, fd);
    if(old != -1) {
        // On Linux the descriptor is released even if close() fails with
        // EINTR, so retrying could close an unrelated, reused descriptor.
        ::close(old);
    }
}

} // namespace miniposix
