# Team notes — MiniPOSIX workshop (17 November 2026)

Team: Rui Passos (+ teammates: add your names)
Fork / PR: https://github.com/RuiPassos/miniposix-workshop/pull/1
AI agent used: Claude Code (instead of OpenCode). Second model for the afternoon exercise: Claude Haiku.

## Before coding: starting familiarity (0 = none, 3 = comfortable)

Ratings, for reflection only. Each teammate adds their own rows.

| Topic | 0–3 | One thing we already knew or needed to look up |
| --- | --- | --- |
| C++ ownership and move semantics | 1 | Needed to look up what a moved-from object must look like (empty, `-1`) and why copying must be disabled |
| POSIX file descriptors and pipes | 0 | Needed to look up what a file descriptor is and when a pipe reader sees end-of-file |
| AI coding agents | 2 | Already used an agent to write code; new: checking its claims with documentation and tests |
| C++ ownership and move semantics | | |
| POSIX file descriptors and pipes | | |
| AI coding agents | | |

## Audit A: attempt log

Environment: our Mac is not Linux, so everything ran in Docker (Ubuntu 24.04, Clang 22.1.8), the same setup as CI.

| Attempt | Failing test or observed behavior | Help needed? | AI mistake or uncertain claim | Evidence and reason for the correction |
| --- | --- | --- | --- | --- |
| 1 | Untouched starter: smoke passes, 11 functional tests fail | No | — | README says this is the expected starting state |
| 2 | Implementation of `UniqueFd` and `Pipe`: all tests pass, no warnings, also with ASan + UBSan | No | The agent claimed that on Linux `close()` must not be retried after `EINTR` | **Not verified yet.** Must be checked in `man 2 close` before we rely on it |
| 3 | `strace` of `Pipe.OwnsAndClosesBothEnds` | No | — | Shows `pipe2([3, 4], O_CLOEXEC)` then `close(4)`, `close(3)`: the descriptors are really closed, not just reset in the wrapper |
| 4 | The agent's first attempt to run the tests in Docker failed (folder not shared with Docker), and a second script had a syntax error | No | Agent tooling mistake, not a POSIX mistake | The agent saw the error output, changed approach (copy files in with `tar`) and re-ran |
| 5 | Our first `git push` was denied | Yes (setup) | — | `origin` still pointed to the instructor's repository; changed it to our fork |
| 6 | First PR was opened against the instructor's repository | Yes (setup) | — | README requires the PR to target `main` in our own fork; closed it and reopened in the fork |

Claim verified against documentation: see the afternoon exercise below (`pipe(7)`, end-of-file with a duplicated write end).

Extra public test added: `Pipe.MoveAssignmentClosesDestinationsOldEnds`. It checks with `fcntl` that the destination's OLD descriptors are closed after `target = std::move(source)`. Checking only the wrapper values (`get()`) would miss a leak, because the values would look correct even if the old descriptors were never closed.

## Audit B: correctness and performance

**Data provenance:** our own runs, `clang22-perf` build, Docker (Ubuntu 24.04, kernel 6.8, 2 vCPUs) on an Apple Silicon Mac, 8 MiB per transfer. Files `bench-results/pipe.csv` (3 repeats) and `bench-results/pipe-10.csv` (10 repeats). They stay local (Git ignores `bench-results/`).

Median throughput (MiB/s):

| Buffer (bytes) | First run (3 repeats) | Second run (10 repeats) | Spread in 10-repeat run (min–max) |
| --- | --- | --- | --- |
| 512 | 414 | 410 | 400–420 |
| 4096 | 849 | 847 | 836–854 |
| 65536 | 931 | 936 | 922–950 |

1. **Highest median throughput?** 65536 bytes (936 MiB/s). The ordering 512 < 4096 < 65536 was the same in every single repeat of both runs. The ranges do not overlap.
2. **What is included in the timing?** Starting the writer thread, every `write()`/`read()` system call, the kernel copying data through the pipe buffer, thread scheduling and context switches, filling the buffer with the test pattern, verifying every received byte, and joining the thread. `UniqueFd` only stores an integer and `get()` returns it, so its cost is tiny compared with the system calls. That is why this is **not** a measurement of the wrapper.
3. **What might change elsewhere?** The CPU, the number of cores, virtualization (we ran inside Docker on macOS), kernel version, pipe buffer size, compiler and flags, and other load on the machine. We did not use `perf stat`, because it is not available in our Docker setup.
4. **What would we change before claiming one variant is generally faster?** Run on several machines and on bare Linux. Use more repeats with warm-up runs. Pin threads to CPUs and keep the machine idle. Record the environment in the CSV. Compare against a baseline without the wrapper (raw `int` descriptors) to isolate its cost. Report confidence intervals instead of single medians.

**Stretch (10 repeats):** the medians of the two runs agree within about 1%. The CSV records `variant, buffer_bytes, total_bytes, seconds, throughput_mib_s, repeat`. It does **not** record CPU model, core count, system load, OS/kernel, compiler or flags, or whether it ran in a VM. Without these, a number cannot be compared with another machine's.

## Afternoon exercise: AI answers and verification workflow

Everything is in [verification/](verification/README.md): the question, both AI answers (Claude Sonnet and Claude Haiku), the `pipe(7)` source, a focused test, `evidence.json` and `workflow.py`, which stops for the human decision.

- **Claim:** a reader sees EOF only after every copy of the write end (including `dup()` copies) is closed.
- **Test:** with a `dup()` copy open, `read()` gives `EAGAIN`; after closing it, `read()` returns `0`.
- **Human verdict:** accept (set by Rui after reading both answers, the source and the test).
- **Doubtful AI claims we found:** answer A mixes in `send()`/`MSG_NOSIGNAL`, which are socket calls, not pipe calls. Answer B uses imprecise wording about "open file descriptions".

## What we could not verify

- `close()` and `EINTR` (attempt 2).
- The `fork()` case and the default SIGPIPE action killing the process (documentation only).
- Performance numbers outside our machine.
- CI green is feedback, not proof of full correctness.

## Architecture discussion: possible extension

**`Subprocess`**: start another program with `fork()` + `exec()` and connect its stdin/stdout through `Pipe` objects. It would introduce these new ownership and error cases:

- Each process must close the pipe ends it does not use. Otherwise the reader never sees EOF, which is exactly the claim we tested in the afternoon.
- Descriptors must be `O_CLOEXEC` so they do not leak into unrelated programs.
- The parent must `waitpid()` the child, otherwise it becomes a zombie. Who owns the child's process ID?
- An error between `fork()` and `exec()` happens in the child, so it must be reported back to the parent (for example through an extra pipe).
- Writing to a child that has exited raises SIGPIPE/EPIPE.
