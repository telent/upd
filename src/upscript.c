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
extern struct export exports[];

#define ERRC(c) lua_pushinteger(L, c);  lua_pushstring(L, #c);  lua_rawset(L, -3)
static void l_errno_table(lua_State *L) {
    lua_newtable(L);
    ERRC(EACCES);
    ERRC(EEXIST);
    ERRC(EFAULT);
    ERRC(EINTR);
    ERRC(EINVAL);
    ERRC(ENOENT);
    ERRC(ENOSPC);
    ERRC(ENOTDIR);
    lua_setglobal(L, "errno");
}
#undef ERRC

static int l_handle_error(lua_State* L) {
    const char * msg = lua_tostring(L, -1);
    luaL_traceback(L, L, msg, 2);
    lua_remove(L, -2); // Remove error/"msg" from stack.
    return 1; // Traceback is returned.
}

int main(int argc, char *argv[]) {
    int status, result;
    lua_State *L;

    /*
     * All Lua contexts are held in this structure. We work with it almost
     * all the time.
     */
    L = luaL_newstate();

    luaL_openlibs(L); /* Load Lua libraries */

    l_errno_table(L);

    lua_pushcfunction(L, l_handle_error);
    /* Load the file containing the script we are going to run */
    status = luaL_loadfile(L, argv[1]);
    if (status) {
        /* If something went wrong, error message is at the top of */
        /* the stack */
        fprintf(stderr, "Couldn't load file: %s\n", lua_tostring(L, -1));
        exit(1);
    }

    lua_newtable(L);
    for(int i=1; i < argc; i++) {
        lua_pushinteger(L, i-1);
        lua_pushstring(L, argv[i]);
        lua_rawset(L, -3);
    }
    lua_setglobal(L, "arg");

    lua_newtable(L);
    for(int i=0; exports[i].name; i++) {
        struct export e = exports[i];
        lua_pushstring(L, e.name);
        lua_pushcfunction(L, e.fn);
        lua_settable(L, -3);
    }
    lua_setglobal(L, "upd");

    result = lua_pcall(L, 0, LUA_MULTRET, -2);
    if (result) {
        fprintf(stderr, "Failed to run script: %s\n", lua_tostring(L, -1));
        exit(1);
    }

    /* Get the returned value at the top of the stack (index -1) */
    int retval = lua_tonumber(L, -1);

    lua_pop(L, 1);  /* Take the returned value out of the stack */
    lua_close(L);   /* Cya, Lua */

    return retval;
}
