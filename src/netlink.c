#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <asm/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <unistd.h>
#include "netlink.h"


int netlink_open_socket() {
    int fd;
    struct sockaddr_nl sa;

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;

    fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    bind(fd, (struct sockaddr *) &sa, sizeof(sa));
    return fd;
}

int netlink_read_message(int fd, struct netlink_message *msg) {
    struct ifinfomsg *rtif;
    int len;
    int payload_bytes;
    char buf[4096 * 8];
    struct nlmsghdr *nh;

    len = read(fd, buf, sizeof (struct nlmsghdr));
    if(len> 0) {
        nh = (struct nlmsghdr *) buf;
        payload_bytes = NLMSG_PAYLOAD(nh, 0);

        robustly(read(fd, buf + NLMSG_HDRLEN, payload_bytes));

        if (nh->nlmsg_type == NLMSG_DONE) {
            printf("got msg done\n");
        }

        if (nh->nlmsg_type == NLMSG_ERROR) {
            printf("got msg error\n");
        }
        if (nh->nlmsg_type == RTM_NEWLINK ||
                nh->nlmsg_type == RTM_DELLINK ||
                nh->nlmsg_type == RTM_GETLINK) {
            rtif = (struct ifinfomsg *) NLMSG_DATA(nh);
            msg->message = (rtif->ifi_flags & IFF_RUNNING) ? LINK_UP : LINK_DOWN;
            if_indextoname(rtif->ifi_index, msg->data.ifname);
        } else {
            msg->message = NOP;
        }
        return 1;
    } else {
        /* this will never happen(sic) in production because reads are
         * blocking and the socket stays open eternally. But we test this
         * code using a file of captured netlink messages, so we need to
         * return falsey when we hit end of file */
        return 0;
    }

}
