#ifndef PATHUTIL_H
#define PATHUTIL_H

// Returns a newly malloc'd string "dir/name" (or just a copy of name if it's
// already absolute). Sized exactly from strlen(dir) + strlen(name) + 2, so it
// can never truncate regardless of PATH_MAX on the platform. Caller must free.
char* join_path(const char* dir, const char* name);

#endif
