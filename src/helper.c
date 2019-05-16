#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    pid_t pid;
    uid_t uid;
    gid_t gid;
    int fds[7];
    char *environ = 0;
    char **environs = 0;

    sscanf(argv[1], "%d", &pid);
    sscanf(argv[2], "%d", &uid);
    sscanf(argv[3], "%d", &gid);

    /* Grab the environment from the faulting process */
    {
        char filename[0xff];
        uint8_t temp_buffer[0x1000];
        ssize_t bytes, filesize = 0;
        int fd;

        sprintf(filename, "/proc/%d/environ", pid);
        if ((fd = open(filename, O_RDONLY)) == -1)
        {
            fprintf(stderr, "Unable to fetch environment from %s\n", filename);
        }

        /* stat(2) reports zero bytes, so do this.  There should be no
           trouble with the size changing inbetween calculation and
           use, because the process has stopped (it is dumping
           core) */
        while ((bytes = read(fd, temp_buffer, 0x1000)) > 0)
        {
            filesize += bytes;
        }
        lseek(fd, 0, SEEK_SET);
        environ = calloc(1, filesize);
        environs = calloc(1, filesize);

        /* Read environment data */
        for (ssize_t i = 0; i < filesize; i += read(fd, environ + i, filesize - i))
            ;
        close(fd);

        /* Parse the environment data */
        for (int i = 0, j = 0; i < filesize && j < filesize; ++i)
        {
            environs[i] = (environ + j);
            while (environ[j] != 0)
            {
                ++j;
            }
            while (j < filesize && environ[j] == 0)
            {
                ++j;
            }
        }
    }

    /* The various types of namespaces (descriptions from from
       nsenter(1) man page) */
    const char *nss[] = {
        "cgroup", /* the cgroup namespace */
        "ipc",    /* the IPC namespace */
        "uts",    /* the UTS namespace */
        "net",    /* the network namespace */
        "pid",    /* the PID namespace */
        "mnt",    /* the mount namespace */
        "user",   /* the user namespace */
    };

    /* Open namespace file descriptors */
    for (int i = 0; i < 7; ++i)
    {
        char filename[0xff];

        sprintf(filename, "/proc/%d/ns/%s", pid, nss[i]);
        if ((fds[i] = open(filename, O_RDONLY)) == -1)
        {
            fprintf(stderr, "Unable to open %s\n", nss[i]);
        }
    }

    /* Change root directory to that of the dumping process. */
    {
        char rootpath[0xff];
        int fd;

        sprintf(rootpath, "/proc/%d/root", pid);
        if ((fd = open(rootpath, O_RDONLY)) == -1)
        {
            fprintf(stderr, "Unable to open %s\n", rootpath);
        }
        if (fchdir(fd) != 0)
        {
            fprintf(stderr, "fchdir failed\n");
        }
        if (chroot(".") != 0)
        {
            fprintf(stderr, "chroot failed\n");
        }
    }

    /* Drop supplementary groups */
    setgroups(0, NULL);

    /* Ensure no shared filesystem attributes */
    unshare(CLONE_FS);

    /* Take up the namespaces of the dumping process. */
    for (int i = 0; i < 7; ++i)
    {
        int retval = setns(fds[i], 0);

        if (retval != 0 && i != 6)
        {
            fprintf(stderr, "Change of %s namespace failed with %d\n", nss[i], errno);
        }
        /* This will fail if the user namespaces are already the same */
        else if (retval != 0 && i == 6)
        {
            fprintf(stderr, "(Warning) change of %s namespace failed with %d\n", nss[i], errno);
        }
        close(fds[i]);
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

    /* Attempt to use the specified handler, otherwise write core file */
    {
        if (execve(argv[4], argv + 4, environs) == -1)
        {
            fprintf(stderr, "Failed to execv %s with %d ... will write core file instead\n", argv[4], errno);

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
    }

    return 0;
}
