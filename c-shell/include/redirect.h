#ifndef REDIRECT_H
#define REDIRECT_H

#include <sys/types.h>
#include "command.h"


// Temporarily redirects the calling process's own stdin/stdout to input_fd/
// output_fd (whichever is >= 0), saving the originals into *saved_stdin /
// *saved_stdout (-1 if that side wasn't redirected). Consumes (closes)
// input_fd/output_fd. Use around a builtin call that must keep running in
// the current process (so it can affect the shell's own state, e.g. hop's
// cwd), then call end_stdio_redirect to restore the real terminal fds.
void begin_stdio_redirect(int input_fd, int output_fd, int* saved_stdin, int* saved_stdout);
void end_stdio_redirect(int saved_stdin, int saved_stdout);

// Opens every file in infiles (NULL-terminated) with O_RDONLY and returns a
// file descriptor that yields their contents back to back, in order, as one
// continuous stream. Caller dup2's this onto STDIN_FILENO in the command's
// child process and closes it afterward.
// Returns -1 and prints "cshell: no such file or directory" if any file
// cannot be opened; in that case the command must not run.
int open_input_stream(char** infiles);

typedef struct
{
    int fd;         // write end of a pipe; dup2 onto STDOUT_FILENO in the command's child. -1 on failure.
    pid_t sink_pid; // process fanning the pipe out to every output file; wait for it after the command exits
} OutputStream;

// Opens every file in outfiles (NULL-terminated by filename) with the mode
// specified per-entry (truncate/append, creating with 0644 if needed), and
// forks a sink process that copies everything written to the returned fd
// into every one of those files.
// On failure, prints "cshell: unable to create file for writing" and
// returns { .fd = -1 }; in that case the command must not run.
OutputStream open_output_stream(OutFileSpec* outfiles);

#endif