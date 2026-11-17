#include "miniposix/pipe.hpp"

#include <cerrno>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>

namespace miniposix {

Pipe Pipe::create() {
    int ends[2] = {-1, -1};
    // O_CLOEXEC stops the descriptors leaking into programs started by exec().
    if(::pipe2(ends, O_CLOEXEC) == -1) {
        // pipe2 creates no descriptors when it fails, so nothing can leak.
        throw std::system_error(errno, std::system_category(), "pipe2");
    }

    // From here on nothing can throw: UniqueFd adoption is noexcept.
    Pipe result;
    result.read_end_.reset(ends[0]);
    result.write_end_.reset(ends[1]);
    return result;
}

} // namespace miniposix
