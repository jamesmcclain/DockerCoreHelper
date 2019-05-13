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
    uid_t uid;
    gid_t gid;

    sscanf(argv[1], "%d", &pid);
    sscanf(argv[2], "%d", &uid);
    sscanf(argv[3], "%d", &gid);

    /* The various types of namespaces (descriptions from from
       nsenter(1) man page) */
    const char *nss[7] = {
        "mnt",    /* the mount namespace */
        "uts",    /* the UTS namespace */
        "ipc",    /* the IPC namespace */
        "net",    /* the network namespace */
        "pid",    /* the PID namespace */
        "user",   /* the user namespace */
        "cgroup", /* the cgroup namespace */
    };

    /* Take up the namespaces of the dumping process.

       XXX Using setns() to change the caller's cgroup namespace does
       not change the caller's cgroup memberships. */
    for (int i = 0; i < 7; ++i)
    {
        char filename[0xff];
        int fd;

        sprintf(filename, "/proc/%d/ns/%s", pid, nss[i]);
        fd = open(filename, O_RDONLY);
        if (setns(fd, 0) != 0)
        {
            fprintf(stderr, "Change of %s namespace failed\n", nss[i]);
        }
        close(fd);
    }

    /* Change root directory to that of the dumping process. */
    {
        char root[0xff];

        sprintf(root, "/proc/%d/root", pid);
        if (chroot(root) != 0)
        {
            fprintf(stderr, "chroot to %s failed\n", root);
        }
    }

    /* Change real and effective user, group */
    if (setregid(gid, gid) != 0)
    {
        fprintf(stderr, "Unable to switch to group %d\n", gid);
    }
    if (setreuid(uid, uid) != 0)
    {
        fprintf(stderr, "Unable to switch to user %d\n", uid);
    }

    return 0;
}
