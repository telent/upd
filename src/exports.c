#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <errno.h>
#include <dirent.h>
#include <errno.h>
#define _GNU_SOURCE
#include <fcntl.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "netlink.h"
#include "exports.h"


static int return_or_error(lua_State *L, int value) {
    if(value >= 0) {
        lua_pushinteger(L, value);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
}

static int lua_objlen(lua_State *L, int index) {
    lua_len(L, index);
    int len = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return len;
}

/* exports */

/* this is a direct paste from http://www.lua.org/pil/26.1.html */

static int l_dir (lua_State *L) {
    DIR *dir;
    struct dirent *entry;
    int i;
    const char *path = luaL_checkstring(L, 1);

    /* open directory */
    dir = opendir(path);
    if (dir == NULL) {  /* error opening the directory? */
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;  /* number of results */
    }

    /* create result table */
    lua_newtable(L);
    i = 1;
    while ((entry = readdir(dir)) != NULL) {
        lua_pushnumber(L, i++);  /* push key */
        lua_pushstring(L, entry->d_name);  /* push value */
        lua_settable(L, -3);
    }

    closedir(dir);
    return 1;  /* table is already on top */
}

void copy_to_array(lua_State *L, int index, const char ** out, int len) {
    for (int i = 1; i <= len; i++) {
        lua_pushinteger(L, i);
        lua_gettable(L, index);
        out[i-1] = lua_tostring(L, -1);
        lua_pop(L, 1);
    }
    out[len] = NULL;
}

static int l_execve(lua_State *L) {
    /* pathname, args, env */
    const char *pathname = luaL_checkstring(L, 1);

    int nargs = lua_objlen(L, 2);
    const char ** argv = alloca((sizeof (char *)) * (1+nargs));
    copy_to_array(L, 2, argv, nargs);

    int nenvs = lua_objlen(L, 3);
    const char ** envp = alloca((sizeof (char *)) * (1+nenvs));
    copy_to_array(L, 3, envp, nenvs);

    /* according to http://stackoverflow.com/questions/190184/execv-and-const-ness
     * the awful casting going on here is actually OK
     */
    int ret = execve(pathname, (char * const *) argv, (char * const *) envp);
    lua_pushinteger(L, ret);
    return 1;
}

static int l_fileno(lua_State *L) {
    FILE **file = (FILE **) luaL_checkudata(L, 1, "FILE*");
    lua_pushinteger(L, fileno(*file));
    return 1;
}

static int l_fork(lua_State *L) {
    pid_t child = fork();
    return return_or_error(L, child);
}

static int l_inotify_add_watch(lua_State *L) {
    int watch_d;
    int fd = lua_tonumber(L, -2);
    const char* name  = lua_tostring(L, -1);
    watch_d = inotify_add_watch(fd, name,
                                IN_CLOSE_WRITE |
                                IN_CREATE |
                                IN_DELETE |
                                IN_DELETE_SELF |
                                IN_MOVED_TO);
    return return_or_error(L, watch_d);
}

static int l_inotify_init(lua_State *L) {
    int fd = inotify_init1(IN_NONBLOCK|IN_CLOEXEC);
    return return_or_error(L, fd);
}

static int l_isdir (lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    struct stat s;
    int ret = stat(path, &s);
    if(ret == 0) {
        lua_pushboolean(L, (s.st_mode & S_IFMT) == S_IFDIR);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
}

static int l_mkdir (lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    int ret = mkdir(path, S_IRWXU | S_IRWXG |S_IRWXO); /* subject to umask */
    return return_or_error(L, ret);
}

static int l_netlink_listener(lua_State *L) {
    int value;
    if(lua_gettop(L) > 0) {
        const char *path = luaL_checkstring(L, 1);
        printf("called with path %s\n", path);
        value = open(path, O_RDONLY);
    } else {
        printf("called without path\n");
        value = netlink_open_socket();
    }
    return return_or_error(L, value);
}

static int l_netlink_read_message(lua_State *L) {
    int fd = lua_tointeger(L, 1);
    struct netlink_message msg;
    if(netlink_read_message(fd, &msg)) {
        lua_newtable(L);

        if(msg.data.ifname) {
            lua_pushstring(L, "interface");
            lua_pushstring(L, msg.data.ifname);
            lua_settable(L, -3);
        }
        if(msg.message == LINK_UP) {
            lua_pushstring(L, "link-status");
            lua_pushstring(L, "up");
            lua_settable(L, -3);
        }
        if(msg.message == LINK_DOWN) {
            lua_pushstring(L, "link-status");
            lua_pushstring(L, "down");
            lua_settable(L, -3);
        }
        return 1;
    } else {
        return 0;
    }
}

static int push_siginfo(lua_State *L, struct signalfd_siginfo *si) {
    lua_createtable(L, 0, 6);

    lua_pushstring(L, "type");
    lua_pushstring(L, "child");
    lua_settable(L, -3);

    lua_pushstring(L, "code");
    lua_pushinteger(L, si->ssi_code);
    lua_settable(L, -3);

    lua_pushstring(L, "pid");
    lua_pushinteger(L, si->ssi_pid);
    lua_settable(L, -3);

    lua_pushstring(L, "utime");
    lua_pushnumber(L, ((double) si->ssi_utime) / sysconf(_SC_CLK_TCK)  );
    lua_settable(L, -3);

    lua_pushstring(L, "stime");
    lua_pushnumber(L, ((double) si->ssi_stime) / sysconf(_SC_CLK_TCK)  );
    lua_settable(L, -3);

    lua_pushstring(L, "status");
    lua_pushinteger(L, si->ssi_status);
    lua_settable(L, -3);

    return 1;
}


static int count_table_entries(lua_State *L, int index) {
    int i;
    lua_pushnil(L);
    for(i=0; lua_next(L, index); i++) {
        lua_pop(L, 1);
    }
    return i;
}

static int l_next_event(lua_State *L) {
    struct signalfd_siginfo si;

    /* receives a sigchld fd and a inotify fd and a table whose keys are
     * pipe fds */
    int sigchld_fd = lua_tonumber(L, 1);
    int inotify_fd = lua_tonumber(L, 2);
    int nfds = count_table_entries(L, 3);
    int timeout_msec = lua_tonumber(L, 4);

    /* stack-allocating the pollfd set seems reasonable in the
     * usual case (maybe two or three child pipe fds to watch)
     * but we do have an opportunity to blow the stack here if a
     * watcher starts eleventy billion children.
     */
    struct pollfd *pollfd = alloca(sizeof (struct pollfd) * (2 + nfds));

    pollfd[0].fd = sigchld_fd;
    pollfd[0].events = POLLIN|POLLPRI;
    pollfd[1].fd = inotify_fd;
    pollfd[1].events = POLLIN|POLLPRI;

    lua_pushnil(L);
    for(int i=2; lua_next(L, 3); i++) {
        pollfd[i].fd = lua_tointeger(L, -2);
        pollfd[i].events = POLLIN|POLLPRI;
        lua_pop(L, 1);
    }

    poll(pollfd, nfds + 2, timeout_msec);

    if(pollfd[0].revents) {
        if(read(pollfd[0].fd, &si, sizeof(struct signalfd_siginfo))) {
            return push_siginfo(L, &si);
        }
        return 0;
    } else if (pollfd[1].revents) {
        struct inotify_event ino_events[6];
        int num;
        if((num = read(pollfd[1].fd, ino_events, sizeof ino_events)) >0) {
            lua_createtable(L, 0, 2);
            lua_pushstring(L, "type");
            lua_pushstring(L, "file");
            lua_settable(L, -3);
            lua_pushstring(L, "watches");
            lua_newtable(L);
            struct inotify_event *e = ino_events;
            while(e < (struct ino_event *) ((char *) ino_events + num)) {
                lua_pushinteger(L, e->wd);
                lua_pushstring(L, e->name);
                lua_settable(L, -3);
                e = e + sizeof(struct inotify_event)+e->len;
            }
            lua_settable(L, -3);
            return 1;
        }
        return 0;
    } else {
        for(int i = 0; i< nfds; i++) {
            if(pollfd[i+2].revents) {
                char buf[1024];
                int fd = pollfd[i+2].fd;
                size_t bytes_read = read(fd, buf, (sizeof buf) -1);
                lua_createtable(L, 0, 2);
                lua_pushstring(L, "type");
                lua_pushstring(L, "stream");
                lua_settable(L, -3);
                lua_pushstring(L, "fd");
                lua_pushinteger(L, fd);
                lua_settable(L, -3);
                lua_pushstring(L, "message");
                if(bytes_read > 0)
                    lua_pushlstring(L, buf, bytes_read);
                else
                    lua_pushnil(L);
                lua_settable(L, -3);
                if (bytes_read < 0) {
                    lua_pushstring(L, "error");
                    lua_pushinteger(L, errno);
                    lua_settable(L, -3);
                }
                return 1;
            }
        }
        return 0;			/* no return for timeout */
    }
}

#define L_CHECK_ERROR(L, value) \
  if(value < 0) { lua_pushnil(L); lua_pushinteger(L, errno); return 2; }

static int l_pfork(lua_State *L) {
    int pipeout[2], pipeerr[2];
    L_CHECK_ERROR(L, pipe(pipeout));
    L_CHECK_ERROR(L, pipe(pipeerr));
    int pid=fork();
    if(pid==0) {
        fclose(stdout);
        fclose(stderr);
        close(pipeout[0]);
        close(pipeerr[0]);
        close(1);
        close(2);
        dup2(pipeout[1], 1);
        dup2(pipeerr[1], 2);

        lua_pushinteger(L, pid);
        return 1;
    } else if(pid > 0) {
        close(pipeout[1]);
        close(pipeerr[1]);
        lua_pushinteger(L, pid);
        lua_pushnil(L);		/* errno return */
        lua_pushinteger(L, pipeout[0]);
        lua_pushinteger(L, pipeerr[0]);
        return 4;
    } else return return_or_error(L, pid);
}


static int l_sigchld_fd(lua_State *L) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    int fd = signalfd(-1, &mask, SFD_NONBLOCK|SFD_CLOEXEC);
    return return_or_error(L, fd);
}

static int l_sleep(lua_State *L) {
    sleep(lua_tonumber(L, -1));
    return 0;
}

static int l_waitpid(lua_State *L) {
    int pid = lua_tointeger(L, 1);
    return return_or_error(L, waitpid(pid, NULL, 0));
}

struct export exports[] = {
    {"dir", l_dir},
    {"execve", l_execve},
    {"fileno", l_fileno},
    {"fork", l_fork},
    {"inotify_add_watch", l_inotify_add_watch},
    {"inotify_init", l_inotify_init},
    {"isdir", l_isdir},
    {"mkdir", l_mkdir},
    {"netlink-listener", l_netlink_listener},
    {"netlink-read-message", l_netlink_read_message},
    {"next_event", l_next_event},
    {"pfork", l_pfork},
    {"sigchld_fd", l_sigchld_fd},
    {"sleep", l_sleep},
    {"waitpid", l_waitpid},
    {0, 0}
};
