#include <stdarg.h>

static void lua_ev_newmetatable(lua_State *L, const char *type_mt) {
    /* create metatable */
    luaL_newmetatable(L, type_mt);
    /* add magic value to metatable. */
    lua_pushlightuserdata(L, (void *)type_mt);
    lua_rawseti(L, -2, OBJ_TYPE_MAGIC_IDX);
    /* save reference to metatable in registry. */
    lua_pushlightuserdata(L, (void *)type_mt);
    lua_pushvalue(L, -2); /* dup metatable. */
    lua_rawset(L, LUA_REGISTRYINDEX);
}

static void lua_ev_getmetatable(lua_State *L, const char *type_mt) {
    /* get metatable from registry. */
    lua_pushlightuserdata(L, (void *)type_mt);
    lua_rawget(L, LUA_REGISTRYINDEX);
}

static void *lua_ev_checkobject(lua_State *L, int idx, const char *type_mt) {
    void *ud;
    ud = lua_touserdata(L, idx);
    if (ud != NULL) {
        /* check object's type by checking it's metatable. */
        if (lua_getmetatable(L, idx)) {
            /* check magic value from metatable. */
            lua_rawgeti(L, -1, OBJ_TYPE_MAGIC_IDX);
            if ( lua_touserdata(L, -1) == type_mt ) {
                lua_pop(L, 2);
                return ud;
            }
        }
    }
    /* wrong type. */
    luaL_typerror(L, idx, type_mt);
    return NULL;
}


/**
 * Create a new "object" with a metatable of tname and allocate size
 * bytes for the object.  Also create an fenv associated with the
 * object.  This fenv is used to keep track of lua objects so that the
 * garbage collector doesn't prematurely collect lua objects that are
 * referenced by the C data structure.
 *
 * [-0, +1, ?]
 */
static void* obj_new(lua_State* L, size_t size, const char* tname) {
    void* obj;

    obj = lua_newuserdata(L, size);
    lua_ev_getmetatable(L, tname);
    lua_setmetatable(L, -2);

    return obj;
}

/* vi:set expandtab ts=4: */
