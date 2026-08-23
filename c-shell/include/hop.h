#ifndef HOP_H
#define HOP_H

// Runs the `hop` builtin for one command invocation.
// `args` is the NULL-terminated list of arguments after "hop" itself
// (may be empty/NULL for a bare "hop"). `home` is the shell's home
// directory (per A1: wherever the shell was started). `previous_cwd`
// points to the shell's tracked "previous CWD" (like OLDPWD) -- hop
// reads it for "-" and updates it on every successful directory change.
void hop_command(char** args, const char* home, char** previous_cwd);

#endif