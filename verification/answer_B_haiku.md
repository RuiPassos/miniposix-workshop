# Answer B — Claude Haiku (separate conversation, no tools)

(1) If the write end is closed and the pipe buffer is empty, read() returns 0, which is end-of-file. It only returns 0 when no write-end descriptor remains open anywhere. If the buffer is empty and a writer still exists, read() blocks instead.

(2) No. The kernel counts open file descriptions, not processes. dup() and fork() both create additional descriptors that refer to the same open file description as the original write end. Closing only the original leaves those copies open, so read() keeps blocking and never returns 0 until every copy in every process is closed. A common bug is a parent that forks a child but does not close its own copy of the write end, so the reader never sees EOF. Each process should close the ends it does not use.

(3) If no read-end descriptors are open anywhere, write() raises SIGPIPE in the calling thread. The default action of SIGPIPE terminates the process. If SIGPIPE is ignored or blocked, write() fails with EPIPE. A writer blocked on a full pipe is woken the same way if the last read end closes. Programs that can outlive their reader typically set SIGPIPE to ignored (many runtimes do this by default) and check write()'s return value for EPIPE.

Source: answered from knowledge of Linux pipe(7), read(2), write(2), and signal(7) semantics; no tools were used, so nothing was checked against the local repository or a live system.
