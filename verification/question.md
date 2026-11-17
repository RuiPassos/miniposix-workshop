# Question asked to both AIs (same text, separate conversations, no tools allowed)

On Linux, I have an anonymous pipe created with pipe().
(1) When the write end is closed, what does read() on the read end return once the pipe buffer is empty?
(2) Is that still true if the write-end descriptor was duplicated with dup() (or inherited by a child after fork()) and only the original descriptor was closed?
(3) What happens when a process write()s to a pipe whose read ends are all closed?

Asked on 2026-11-17 through Claude Code subagents: answer A = Claude Sonnet, answer B = Claude Haiku.
