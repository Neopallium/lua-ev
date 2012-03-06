#include <assert.h>
#include <ev.h>
#include <lauxlib.h>
#include <lua.h>

#include "lua_ev.h"

static char lua_ev_loop_mt[]   = "ev{loop}";
static char lua_ev_io_mt[]     = "ev{io}";
static char lua_ev_timer_mt[]  = "ev{timer}";
static char lua_ev_signal_mt[] = "ev{signal}";
static char lua_ev_idle_mt[]   = "ev{idle}";
static char lua_ev_child_mt[]  = "ev{child}";
static char lua_ev_stat_mt[]   = "ev{stat}";

/* We make everything static, so we just include all *.c files in a
 * single compilation unit. */
#include "obj_lua_ev.c"
#include "loop_lua_ev.c"
#include "watcher_lua_ev.c"
#include "io_lua_ev.c"
#include "timer_lua_ev.c"
#include "signal_lua_ev.c"
#include "idle_lua_ev.c"
#include "child_lua_ev.c"
#include "stat_lua_ev.c"

static const luaL_reg R[] = {
    {"version", version},
    {NULL, NULL},
};

static char *traceback_key = "LUA_EV_TRACEBACK_KEY";

static void push_traceback(lua_State *L) {
    lua_pushlightuserdata(L, traceback_key);
    lua_gettable(L, LUA_REGISTRYINDEX); /* registry[<traceback_key>] */
}

static void save_traceback(lua_State *L) {
    lua_pushlightuserdata(L, traceback_key);
    /* save reference to 'debug.traceback' */
    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    if ( !lua_istable(L, -1) ) {
        luaL_error(L, "Can't get global 'debug'");
    }

    lua_getfield(L, -1, "traceback");
    if ( !lua_isfunction(L, -1) ) {
        luaL_error(L, "Can't get 'debug.trackeback' function");
    }
    lua_remove(L, -2); /* remove 'debug' table from stack. */
    lua_settable(L, LUA_REGISTRYINDEX); /* registry[<traceback_key>] = debug.traceback */
}

/**
 * Entry point into the 'ev' lua library.  Validates that the
 * dynamically linked libev version matches, creates the object
 * registry, and creates the table returned by require().
 */
LUALIB_API int luaopen_ev(lua_State *L) {

    assert(ev_version_major() == EV_VERSION_MAJOR &&
           ev_version_minor() >= EV_VERSION_MINOR);

    save_traceback(L);

    luaL_register(L, "ev", R);

#define CONSTANT(def, name) do { \
    lua_pushinteger(L, def); \
    lua_setfield(L, -2, name); \
} while(0);

#if EV_VERSION_MAJOR >= 4
    CONSTANT(EVRUN_NOWAIT, "NOWAIT");
    CONSTANT(EVRUN_ONCE, "ONCE");
    CONSTANT(EVBREAK_CANCEL, "CANCEL");
    CONSTANT(EVBREAK_ONE, "ONE");
    CONSTANT(EVBREAK_ALL, "ALL");
#else
    /* use names from 4.x */
    CONSTANT(EVLOOP_NONBLOCK, "NOWAIT");
    CONSTANT(EVLOOP_ONESHOT, "ONCE");
    CONSTANT(EVUNLOOP_CANCEL, "CANCEL");
    CONSTANT(EVUNLOOP_ONE, "ONE");
    CONSTANT(EVUNLOOP_ALL, "ALL");
#endif
#undef CONSTANT

    luaopen_ev_loop(L);
    lua_setfield(L, -2, "Loop");

    luaopen_ev_timer(L);
    lua_setfield(L, -2, "Timer");

    luaopen_ev_io(L);
    lua_setfield(L, -2, "IO");

    luaopen_ev_signal(L);
    lua_setfield(L, -2, "Signal");

    luaopen_ev_idle(L);
    lua_setfield(L, -2, "Idle");

    luaopen_ev_child(L);
    lua_setfield(L, -2, "Child");

    luaopen_ev_stat(L);
    lua_setfield(L, -2, "Stat");

#define CONSTANT(name) do { \
    lua_pushinteger(L, EV_ ## name); \
    lua_setfield(L, -2, #name); \
} while(0);

    CONSTANT(READ);
    CONSTANT(WRITE);
    CONSTANT(TIMEOUT);
    CONSTANT(SIGNAL);
    CONSTANT(IDLE);
    CONSTANT(CHILD);
    CONSTANT(STAT);
    CONSTANT(MINPRI);
    CONSTANT(MAXPRI);

#undef CONSTANT
    return 1;
}

/**
 * Push the major and minor version of libev onto the stack.
 *
 * [+2, -0, -]
 */
static int version(lua_State *L) {
    lua_pushnumber(L, ev_version_major());
    lua_pushnumber(L, ev_version_minor());
    return 2;
}

/**
 * Taken from lua.c out of the lua source distribution.  Use this
 * function when doing lua_pcall().
 */
static int traceback(lua_State *L) {
    if ( !lua_isstring(L, 1) ) return 1;

    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    if ( !lua_istable(L, -1) ) {
        lua_pop(L, 1);
        return 1;
    }

    lua_getfield(L, -1, "traceback");
    if ( !lua_isfunction(L, -1) ) {
        lua_pop(L, 2);
        return 1;
    }

    lua_pushvalue(L, 1);    /* pass error message */
    lua_pushinteger(L, 2);  /* skip this function and traceback */
    lua_call(L, 2, 1);      /* call debug.traceback */
    return 1;
}

/* vi:set expandtab ts=4: */
