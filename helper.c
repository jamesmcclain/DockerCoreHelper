#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    pid_t pid;

    sscanf(argv[1], "%d", &pid);

    /* The various types of namespaces (descriptions from from nsenter(1) man page) */
    const char *nss[7] = {
        "mnt",    /* the mount namespace */
        "uts",    /* the UTS namespace */
        "ipc",    /* the IPC namespace */
        "net",    /* the network namespace */
        "pid",    /* the PID namespace */
        "user",   /* the user namespace */
        "cgroup", /* the cgroup namespace */
    };

    /* XXX Using setns() to change the caller's cgroup namespace does not change the caller's cgroup memberships. */
    for (int i = 0; i < 7; ++i)
    {
        char filename[0xff];
        int fd;

        sprintf(filename, "/proc/%d/ns/%s", pid, nss[i]);
        fd = open(filename, O_RDONLY);
        if (setns(fd, 0) != 0)
        {
            fprintf(stderr, "%s namespace failed\n", nss[i]);
        }
        close(fd);
    }
    return 0;
}
