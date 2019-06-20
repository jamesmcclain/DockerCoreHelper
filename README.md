# Intro #

This project makes it easier to work with core files generated from within Docker containers.
The default behavior is to write to a file within the container when [`/proc/sys/kernel/core_pattern`](http://man7.org/linux/man-pages/man5/core.5.html) is set to a file (e.g. `/tmp/core.%p`), but to run a helper program only on the host when a helper program is specified (e.g. `|/usr/bin/hello_world`).
Sometimes it is useful to run the helper within the Docker container of the faulting process, but motivated searches have not turned up a ready solution.
This project provides a helper which runs on the host and delegates to a specified helper program running within the container, and that delegate runs at the level of privilege of the faulting process.
This allows conveniences such as automatically uploading core files generated in cloud-hosted containers to S3 so that those cores can be downloaded for local analysis.

# How to Use #

## Compile ##

Type `make -C src` from the root of the project.
A program called `src/docker_file_helper` will be generated.

## Setup ##

Put `docker_file_helper` on the host (e.g. at `/opt/bin/docker_file_helper`).

Make sure that the desired helper script exists in the Docker container or image of interest (e.g. at `/opt/bin/uploader.sh`).

## Enable ##

Enable the helper on the host so that it can delegate to the helper in the container (e.g. `echo '|/opt/bin/docker_core_helper %P %u %g /opt/bin/uploader.sh s3://my-bucket-for-cores/ %P %u %g' | sudo tee /proc/sys/kernel/core_pattern`).

If no helper program is found, the default behavior is for no action to be taken.
If the environment variable `DOCKERCOREHELPER_FALLBACK_CORE` is defined, then a core file will be generated with a pattern equivalent to `/tmp/core.%P.%u.%g` (with `%P`, `%u`, and `%g` as defined in the `core(5)` manpage).
That behavior pertains to core files generated inside Docker containers or outside if the helper cannot be found in the view of the filesystem seen by the faulting process.
