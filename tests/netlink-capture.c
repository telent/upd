/**************
 *
 * This is a small program to capture netlink messages to disk so that
 * we can test that kind of event more easily than by physically
 * unplugging ethernet cables
 *
 * 1) Compile this with
 *
 *   $ cc -o netlink-capture -I ../src netlink-capture.c ../src/netlink.c
 *
 * 2) start a qemu vm in which the output binary is available
 *
 * 3) in the VM run "./netlink-capture netlink-messages.raw"
 *
 * 4) switch to the qemu console (Ctrl-Alt-2)
 *
 * 5) run "info network" to get the name of the emulated ethernet
 * device. For me it's "virtio-net-pci.0", if yours is different then
 * amend step 6 appropriately
 *
 * 6) run "set_link virtio-net-pci.o off" and "set_link virtio-net-pci.o on"
 * a few times
 *
 * 7) switch back to the qemu display (Ctrl-Alt-1), and Ctrl-C the
 * running program.
 *
 * 8) copy the generated netlink-messages.raw file back to the host
 *
 **************/

#include "netlink.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int fd;
    int len;
    char buf[4096];
    int outfd;

    if(argc != 2) {
        fprintf(stderr, "Missing output file name\n");
        exit(1);
    }

    outfd = open(argv[1], O_CREAT | O_RDWR, 0644);
    if(outfd < 0) {
      perror("open");
      exit(1);
    }
    fd = netlink_open_socket();

    while(1) {
        len = read(fd, buf, sizeof buf);
        robustly(write(outfd, buf, len));
        robustly(write(1, ".", 1));
        fsync(outfd);
    }

    return 0;
}
