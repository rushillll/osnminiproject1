#ifndef EXECUTOR_H
#define EXECUTOR_H

// Resolves token (a bare name, PATH-searched, or '%'-prefixed to skip cwd
// lookup) to an absolute executable path. Returns NULL if nothing is
// executable. Caller must free a non-NULL result.
char* find_executable(const char* token);

void execute_command(char** argv, int input_fd, int output_fd); // -1 for either means "not redirected"

#endif