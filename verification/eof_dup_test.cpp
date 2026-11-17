// Focused check for the afternoon verification exercise.
// Claim under test: a pipe reader sees end-of-file (read() == 0) only after
// EVERY descriptor referring to the write end is closed, including dup() copies.
//
// Build and run on Linux from the repository root:
//   clang++-22 -std=c++23 -Iinclude src/unique_fd.cpp src/pipe.cpp \
//       verification/eof_dup_test.cpp -o /tmp/eof_dup_test && /tmp/eof_dup_test

#include "miniposix/pipe.hpp"
#include "miniposix/unique_fd.hpp"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if(!ok) ++failures;
}

struct ReadResult {
    ssize_t count;
    int error;
};

ReadResult read_one(int fd) {
    char byte = '\0';
    errno = 0;
    const ssize_t count = ::read(fd, &byte, 1);
    return {count, count < 0 ? errno : 0};
}

} // namespace

int main() {
    // Part 1: EOF with a duplicated write end.
    auto pipe = miniposix::Pipe::create();
    const int reader = pipe.read_end().get();
    // Non-blocking, so "would block" shows up as EAGAIN instead of hanging.
    ::fcntl(reader, F_SETFL, ::fcntl(reader, F_GETFL) | O_NONBLOCK);

    miniposix::UniqueFd writer_copy{::dup(pipe.write_end().get())};
    check(writer_copy.get() >= 0, "dup() made a second write-end descriptor");

    pipe.write_end().reset(); // close only the ORIGINAL write end
    const auto with_copy_open = read_one(reader);
    std::printf("      read() with dup copy still open -> %zd (%s)\n", with_copy_open.count,
                with_copy_open.count < 0 ? std::strerror(with_copy_open.error) : "no errno");
    check(with_copy_open.count == -1 && with_copy_open.error == EAGAIN,
          "no EOF while a dup() copy of the write end is open (EAGAIN = would block)");

    writer_copy.reset(); // close the LAST write-end descriptor
    const auto all_closed = read_one(reader);
    std::printf("      read() after closing every write end -> %zd\n", all_closed.count);
    check(all_closed.count == 0, "EOF (read returns 0) once every write end is closed");

    // Part 2: writing after every read end is closed.
    std::signal(SIGPIPE, SIG_IGN); // otherwise the default action kills the process
    auto second = miniposix::Pipe::create();
    second.read_end().reset();
    const char byte = 'x';
    errno = 0;
    const ssize_t written = ::write(second.write_end().get(), &byte, 1);
    const int write_error = errno;
    std::printf("      write() with no reader (SIGPIPE ignored) -> %zd (%s)\n", written,
                std::strerror(write_error));
    check(written == -1 && write_error == EPIPE, "write fails with EPIPE when SIGPIPE is ignored");

    std::printf("%s\n", failures == 0 ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");
    return failures == 0 ? 0 : 1;
}
