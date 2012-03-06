#include <stdint.h>

static char watcher_magic[] = "ev{watcher}";

/**
 * Add watcher specific methods to the table on the top of the lua
 * stack.
 *
 * [-0, +0, ?]
 */
static int add_watcher_mt(lua_State *L, luaL_reg* methods, const char* tname) {

    static luaL_reg common_methods[] = {
        { "is_active",     watcher_is_active },
        { "is_pending",    watcher_is_pending },
        { "clear_pending", watcher_clear_pending },
        { "callback",      watcher_callback },
        { "priority",      watcher_priority },
        { "shadow",        watcher_shadow },
        { NULL, NULL }
    };
    lua_ev_newmetatable(L, tname);

    /* Mark this as being a watcher: */
    lua_pushlightuserdata(L, (void*)watcher_magic);
    lua_rawseti(L, -2, WATCHER_TYPE_MAGIC_IDX);

    /* create methods table. */
    lua_createtable(L, 0, 10);
        /* add methods to table. */
    luaL_register(L, NULL, common_methods);
    luaL_register(L, NULL, methods);

    /* create __index/__newindex closures in metatable. */
    lua_pushcclosure(L, watcher_index, 1); /* use methods table in upval 1. */
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, watcher_newindex);
    lua_setfield(L, -2, "__newindex");

    /* hide metatable. */
    lua_pushboolean(L, 0);
    lua_setfield(L, -2, "__metatable");
    return 1;
}

static ev_watcher* lua_ev_checkwatcher(lua_State *L, int idx, const char *type_mt) {
    char *watcher = lua_ev_checkobject(L, idx, type_mt);
    if (watcher != NULL) watcher += WATCHER_DATA_SIZE;
    return (ev_watcher *)watcher;
}

/**
 * Checks that we have a watcher at watcher_i index by validating the
 * metatable has the <watcher_magic> field set to true that is simply
 * used to mark a metatable as being a "watcher".
 *
 * [-0, +0, ?]
 */
static ev_watcher* check_watcher(lua_State *L, int watcher_i) {
    char *watcher = lua_touserdata(L, watcher_i);
    if ( watcher != NULL ) { /* Got a userdata? */
        if ( lua_getmetatable(L, watcher_i) ) { /* got a metatable? */
            lua_rawgeti(L, -1, WATCHER_TYPE_MAGIC_IDX);

            if ( lua_touserdata(L, -1) == watcher_magic ) {
                lua_pop(L, 2);
                return (ev_watcher*)(watcher + WATCHER_DATA_SIZE);
            }
        }
    }
    luaL_typerror(L, watcher_i, "ev{io,timer,signal,idle}");
    return NULL;
}

/**
 * Test if the watcher is active.
 *
 * Usage:
 *   bool = watcher:is_active()
 *
 * [+1, -0, e]
 */
static int watcher_is_active(lua_State *L) {
    lua_pushboolean(L, ev_is_active(check_watcher(L, 1)));
    return 1;
}

/**
 * Test if the watcher is pending.
 *
 * Usage:
 *   bool = watcher:is_pending()
 *
 * [+1, -0, e]
 */
static int watcher_is_pending(lua_State *L) {
    lua_pushboolean(L, ev_is_pending(check_watcher(L, 1)));
    return 1;
}

/**
 * If the watcher is pending, return the revents and clear the pending
 * status (so the watcher callback won't be called).
 *
 * Usage:
 *   revents = watcher:clear_pending(loop)
 *
 * [+1, -0, e]
 */
static int watcher_clear_pending(lua_State *L) {
    lua_pushnumber(L, ev_clear_pending(*check_loop_and_init(L, 2), check_watcher(L, 1)));
    return 1;
}

/**
 * Implement the new function on all the watcher objects.  The first
 * element on the stack must be the callback function.  The new
 * "watcher" is now at the top of the stack.
 *
 * [+1, -0, ?]
 */
static ev_watcher* watcher_new(lua_State* L, size_t size, const char* lua_type) {
    char*  obj;
    ev_watcher* watcher;
    lua_ev_watcher_data *wdata;

    luaL_checktype(L, 1, LUA_TFUNCTION);

    obj = obj_new(L, WATCHER_DATA_SIZE + size, lua_type);

    /* create fenv table for watcher. */
    lua_createtable(L, 2, 0);

    /* save reference to callback in fenv table. */
    lua_pushvalue(L, 1); /* dup watcher callback function. */
    lua_rawseti(L, -2, WATCHER_FN);

    /* set watcher's fenv table. */
    lua_setfenv(L, -2);

    wdata = (lua_ev_watcher_data*)obj;
    wdata->watcher_ref = LUA_NOREF;
    wdata->flags = 0;

    watcher = (ev_watcher*)(obj + WATCHER_DATA_SIZE);

    return watcher;
}

/**
 * Implements the callback function on all the watcher objects.  This
 * will be indirectly called by the libev event loop implementation.
 *
 * TODO: Custom error handlers?  Currently, any error in a callback
 * will print the error to stderr and things will "go on".
 *
 * [+0, -0, m]
 */
static void watcher_cb(struct ev_loop *loop, void *watcher, int revents) {
    lua_State* L       = ev_userdata(loop);
    lua_ev_watcher_data* wdata = GET_WATCHER_DATA(watcher);
    int        result;

    result = lua_checkstack(L, 5);
    assert(result != 0 /* able to allocate enough space on lua stack */);

    /* push 'debug.traceback' function. */
    push_traceback(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, wdata->watcher_ref);
    lua_getfenv(L, -1);
    /* STACK: <traceback>, <watcher>, <watcher fenv> */
    /* insert <callback>, <loop> before <watcher> */
    lua_rawgeti(L, -1, WATCHER_FN);
    lua_insert(L, -3);
    lua_rawgeti(L, -1, WATCHER_LOOP);
    lua_insert(L, -3);
    lua_pop(L, 1); /* pop <watcher fenv> */

    /* STACK: <traceback>, <watcher fn>, <loop>, <watcher> */

    if ( !ev_is_active(watcher) ) {
        /* Must remove "stop"ed watcher from loop: */
        loop_stop_watcher(L, loop, wdata, 1);
    }

    /* push revents */
    lua_pushinteger(L, revents);

    /* STACK: <traceback>, <watcher fn>, <loop>, <watcher>, <revents> */
    if ( lua_pcall(L, 3, 0, -5) ) {
        /* TODO: Enable user-specified error handler! */
        fprintf(stderr, "CALLBACK FAILED: %s\n",
                lua_tostring(L, -1));
        lua_pop(L, 2); /* pop error string & traceback function. */
    } else {
        lua_pop(L, 1); /* pop traceback function. */
    }
}

/**
 * Get/set the watcher callback.  If passed a new_callback, then the
 * old_callback will be returned.  Otherwise, just returns the current
 * callback function.
 *
 * Usage:
 *   old_callback = watcher:callback([new_callback])
 *
 * [+1, -0, e]
 */
static int watcher_callback(lua_State *L) {
    ev_watcher *watcher = check_watcher(L, 1);
    lua_ev_watcher_data* wdata = GET_WATCHER_DATA(watcher);
    int has_fn = lua_gettop(L) > 1;

    lua_getfenv(L, 1);
    lua_rawgeti(L, -1, WATCHER_FN); /* get current callback. */

    if ( has_fn ) {
        luaL_checktype(L, 2, LUA_TFUNCTION);
        lua_pushvalue(L, 2);
        lua_rawseti(L, -3, WATCHER_FN); /* set new callback. */
    }
    /* return current/old callback. */
    return 1;
}

/**
 * Get/set the watcher priority.  If passed a new_priority, then the
 * old_priority will be returned.  Otherwise, just returns the current
 * priority.
 *
 * Usage:
 *   old_priority = watcher:priority([new_priority])
 *
 * [+1, -0, e]
 */
static int watcher_priority(lua_State *L) {
    int has_pri = lua_gettop(L) > 1;
    ev_watcher *w = check_watcher(L, 1);
    int old_pri = ev_priority(w);

    if ( has_pri ) ev_set_priority(w, luaL_checkint(L, 2));
    lua_pushinteger(L, old_pri);
    return 1;
}

/**
 * Get/set the watcher shadow.  If passed a new_shadow, then the
 * old_shadow will be returned.  Otherwise, just returns the current
 * shadow function.
 *
 * Usage:
 *   old_shadow = watcher:shadow([new_shadow])
 *
 * [+1, -0, e]
 */
static int watcher_shadow(lua_State *L) {
    ev_watcher *watcher = check_watcher(L, 1);
    lua_ev_watcher_data* wdata = GET_WATCHER_DATA(watcher);
    int has_param = lua_gettop(L) > 1;

    lua_getfenv(L, 1);
    lua_rawgeti(L, -1, WATCHER_SHADOW); /* get current shadow. */

    if ( has_param ) {
        /* update has_shadow flag. */
        if ( lua_isnil(L, 2) ) {
            wdata->flags &= ~WATCHER_FLAG_HAS_SHADOW;
        } else {
            wdata->flags |= WATCHER_FLAG_HAS_SHADOW;
        }
        /* set new shadow. */
        lua_pushvalue(L, 2);
        lua_rawseti(L, -3, WATCHER_SHADOW); /* set new shadow. */
    }
    /* return current/old shadow. */
    return 1;
}

/**
 * Lazily create the shadow table, and provide write access to this
 * shadow table.
 *
 * [-0, +0, ?]
 */
static int watcher_newindex(lua_State *L) {
    ev_watcher *watcher = check_watcher(L, 1);
    lua_ev_watcher_data* wdata = GET_WATCHER_DATA(watcher);

    lua_settop(L, 3);
    /* STACK: <watcher>, <key>, <value> */

    if ( (wdata->flags & WATCHER_FLAG_HAS_SHADOW) ) {
        /* get existing shadow table. */
        lua_getfenv(L, 1);
        lua_rawgeti(L, -1, WATCHER_SHADOW);
        lua_remove(L, -2); /* remove fenv table. */
    } else {
        /* Lazily create the shadow table. */
        lua_getfenv(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_rawseti(L, -3, WATCHER_SHADOW);
        wdata->flags |= WATCHER_FLAG_HAS_SHADOW;
    }
    /* STACK: <watcher>, <key>, <value>, <shadow> */

    /* h(table, key,value) */
    lua_replace(L, 1); /* replace <watcher> userdata with shadow table. */
    /* STACK: <shadow>, <key>, <value> */
    lua_settable(L, 1);
    return 0;
}

/**
 * Provide read access to the shadow table.
 *
 * [-0, +1, ?]
 */
static int watcher_index(lua_State *L) {
    ev_watcher *watcher;
    lua_ev_watcher_data* wdata;

    /* STACK: <watcher>, <key> */

    /* first lookup method in methods table (upval 1). */
    lua_pushvalue(L, 2); /* dup <key> */
    /* STACK: <watcher>, <key>, <key> */
    lua_gettable(L, lua_upvalueindex(1));
    /* STACK: <watcher>, <key>, <value> */
    if ( ! lua_isnil(L, -1) ) return 1;
    lua_pop(L, 1); /* pop <nil> */
    /* STACK: <watcher>, <key> */

    watcher = check_watcher(L, 1);
    wdata = GET_WATCHER_DATA(watcher);

    /* next check shadow table if the watcher has one. */
    if ( (wdata->flags & WATCHER_FLAG_HAS_SHADOW) ) {
        /* get shadow table from fenv. */
        lua_getfenv(L, 1);
        lua_rawgeti(L, -1, WATCHER_SHADOW);
        lua_remove(L, -2); /* remove fenv table. */

        lua_pushvalue(L, 2);
        /* STACK: <watcher>, <key>, <shadow>, <key> */
        lua_gettable(L, -2);
        /* STACK: <watcher>, <key>, <shadow>, <value> */
    } else {
        lua_pushnil(L); /* no shadow table, just push nil. */
    }
    return 1;
}

/* vi:set expandtab ts=4: */
