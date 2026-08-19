#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>

int main(void)
{
    char hostname[256];
    char homedirectory[1024];

    struct passwd *uid_pw = getpwuid(getuid());
    char *username = uid_pw->pw_name;

    gethostname(hostname, sizeof(hostname));
    getcwd(homedirectory, sizeof(homedirectory));

}