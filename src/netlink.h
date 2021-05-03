#include <net/if.h>
#include <stdio.h>

struct netlink_message {
    enum { NOP, LINK_UP, LINK_DOWN } message;
    union {
        char ifname[IF_NAMESIZE];
    } data;
};

inline void robustly(int value) {
    if(value) return;
    perror("netlink");
}

int netlink_open_socket(void);
