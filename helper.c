#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
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

    /* Write core file */
    {
        ssize_t count = -1;
        uint8_t buffer[0x1000];
        int fd;
        char path[0xff];

        sprintf(path, "/tmp/core.%d.%d.%d", pid, uid, gid);
        if ((fd = open(path, O_WRONLY | O_CREAT | O_SYNC, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP)) == -1)
        {
            fprintf(stderr, "Unable to open %s for writing\n", path);
        }

        while ((count = read(0, buffer, 0x1000)) > 0)
        {
            if (write(fd, buffer, count) == -1)
            {
                fprintf(stderr, "Unable to write %ld bytes to %s due to %d\n", count, path, errno);
            }
        }
        close(fd);
    }

    return 0;
}
