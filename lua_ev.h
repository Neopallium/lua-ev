/**
 * This is a private header file.  If you use it outside of this
 * package, then you will be surprised by any changes made.
 *
 * Documentation for functions declared here is with the function
 * implementation.
 */

/**
 * Compatibility defines
 */

#if EV_VERSION_MAJOR < 3 || (EV_VERSION_MAJOR == 3 && EV_VERSION_MINOR < 7)
#  warning lua-ev requires version 3.7 or newer of libev
#endif

/**
 * Define the names used for the metatables.
 */
#define LOOP_MT    lua_ev_loop_mt
#define IO_MT      lua_ev_io_mt
#define TIMER_MT   lua_ev_timer_mt
#define SIGNAL_MT  lua_ev_signal_mt
#define IDLE_MT    lua_ev_idle_mt
#define CHILD_MT   lua_ev_child_mt
#define STAT_MT    lua_ev_stat_mt

/**
 * Special token to represent the uninitialized default loop.  This is
 * so we can defer initializing the default loop for as long as
 * possible.
 */
#define UNINITIALIZED_DEFAULT_LOOP (struct ev_loop*)1

typedef struct lua_ev_watcher_data lua_ev_watcher_data;

struct lua_ev_watcher_data {
    int watcher_ref;
    int flags;
};
#define ALIGN_SIZE(s, n) (((s) + ((n) - 1)) & -(n))
#define WATCHER_DATA_SIZE ALIGN_SIZE(sizeof(lua_ev_watcher_data), sizeof(void *))
#define GET_WATCHER_DATA(watcher) (lua_ev_watcher_data*)(((char*)watcher) - WATCHER_DATA_SIZE)
#define WATCHER_FLAG_IS_DAEMON   1
#define WATCHER_FLAG_HAS_SHADOW  2

/**
 * The location in the fenv of the watcher that contains the callback
 * function.
 */
#define WATCHER_FN 1

/**
 * The location in the fenv of the watcher that contains the ev_loop reference.
 */
#define WATCHER_LOOP 2

/**
 * The location in the fenv of the shadow table.
 */
#define WATCHER_SHADOW 3

/**
 * Various "check" functions simply call lua_ev_checkobject() and do the
 * appropriate casting, with the exception of check_watcher which is
 * implemented as a C function.
 */

/**
 * If there is any chance that the loop is not initialized yet, then
 * please use check_loop_and_init() instead!  Also note that the
 * loop userdata is a pointer, so don't forget to de-reference the
 * result.
 */
#define OBJ_TYPE_MAGIC_IDX 1
#define WATCHER_TYPE_MAGIC_IDX (OBJ_TYPE_MAGIC_IDX+1)
static void lua_ev_newmetatable(lua_State *L, const char *type_mt);
static void lua_ev_getmetatable(lua_State *L, const char *type_mt);
static void* lua_ev_checkobject(lua_State *L, int idx, const char *type_mt);
static ev_watcher* lua_ev_checkwatcher(lua_State *L, int idx, const char *type_mt);
#define check_loop(L, narg)                                      \
    ((struct ev_loop**)    lua_ev_checkobject((L), (narg), LOOP_MT))

#define check_timer(L, narg)                                     \
    ((ev_timer*)    lua_ev_checkwatcher((L), (narg), TIMER_MT))

#define check_io(L, narg)                                        \
    ((ev_io*)       lua_ev_checkwatcher((L), (narg), IO_MT))

#define check_signal(L, narg)                                   \
    ((ev_signal*)   lua_ev_checkwatcher((L), (narg), SIGNAL_MT))

#define check_idle(L, narg)                                      \
    ((ev_idle*)     lua_ev_checkwatcher((L), (narg), IDLE_MT))

#define check_child(L, narg)                                      \
    ((ev_child*)     lua_ev_checkwatcher((L), (narg), CHILD_MT))

#define check_stat(L, narg)                                      \
    ((ev_stat*)     lua_ev_checkwatcher((L), (narg), STAT_MT))


/**
 * Copied from the lua source code lauxlib.c.  It simply converts a
 * negative stack index into a positive one so that if the stack later
 * grows or shrinks, the index will not be effected.
 */


/**
 * Generic functions:
 */
static int               version(lua_State *L);
static void              push_traceback(lua_State *L);

/**
 * Loop functions:
 */
static int               luaopen_ev_loop(lua_State *L);
static int               create_loop_mt(lua_State *L);
static struct ev_loop**  loop_alloc(lua_State *L);
static struct ev_loop**  check_loop_and_init(lua_State *L, int loop_i);
static int               loop_new(lua_State *L);
static int               loop_delete(lua_State *L);
static void              loop_start_watcher(lua_State* L, struct ev_loop *loop,
                            lua_ev_watcher_data* wdata, int loop_i, int watcher_i, int is_daemon);
static void              loop_stop_watcher(lua_State* L, struct ev_loop *loop,
                            lua_ev_watcher_data* wdata, int watcher_i);
static int               loop_is_default(lua_State *L);
static int               loop_iteration(lua_State *L);
static int               loop_depth(lua_State *L);
static int               loop_now(lua_State *L);
static int               loop_update_now(lua_State *L);
static int               loop_run(lua_State *L);
static int               loop_break(lua_State *L);
static int               loop_backend(lua_State *L);
static int               loop_fork(lua_State *L);

/**
 * Object functions:
 */
static void*             obj_new(lua_State* L, size_t size, const char* tname);

/**
 * Watcher functions:
 */
static int               add_watcher_mt(lua_State *L, luaL_reg* methods, const char* tname);
static int               watcher_is_active(lua_State *L);
static int               watcher_is_pending(lua_State *L);
static int               watcher_clear_pending(lua_State *L);
static ev_watcher*       watcher_new(lua_State* L, size_t size, const char* lua_type);
static int               watcher_callback(lua_State *L);
static int               watcher_priority(lua_State *L);
static int               watcher_shadow(lua_State *L);
static int               watcher_newindex(lua_State *L);
static int               watcher_index(lua_State *L);
static void              watcher_cb(struct ev_loop *loop, void *watcher, int revents);
static ev_watcher*       check_watcher(lua_State *L, int watcher_i);

/**
 * Timer functions:
 */
static int               luaopen_ev_timer(lua_State *L);
static int               create_timer_mt(lua_State *L);
static int               timer_new(lua_State* L);
static void              timer_cb(struct ev_loop* loop, ev_timer* timer, int revents);
static int               timer_again(lua_State *L);
static int               timer_stop(lua_State *L);
static int               timer_start(lua_State *L);
static int               timer_clear_pending(lua_State *L);

/**
 * IO functions:
 */
static int               luaopen_ev_io(lua_State *L);
static int               create_io_mt(lua_State *L);
static int               io_new(lua_State* L);
static void              io_cb(struct ev_loop* loop, ev_io* io, int revents);
static int               io_stop(lua_State *L);
static int               io_start(lua_State *L);
static int               io_getfd(lua_State *L);

/**
 * Signal functions:
 */
static int               luaopen_ev_signal(lua_State *L);
static int               create_signal_mt(lua_State *L);
static int               signal_new(lua_State* L);
static void              signal_cb(struct ev_loop* loop, ev_signal* sig, int revents);
static int               signal_stop(lua_State *L);
static int               signal_start(lua_State *L);

/**
 * Idle functions:
 */
static int               luaopen_ev_idle(lua_State *L);
static int               create_idle_mt(lua_State *L);
static int               idle_new(lua_State* L);
static void              idle_cb(struct ev_loop* loop, ev_idle* idle, int revents);
static int               idle_stop(lua_State *L);
static int               idle_start(lua_State *L);

/**
 * Child functions:
 */
static int               luaopen_ev_child(lua_State *L);
static int               create_child_mt(lua_State *L);
static int               child_new(lua_State* L);
static void              child_cb(struct ev_loop* loop, ev_child* sig, int revents);
static int               child_stop(lua_State *L);
static int               child_start(lua_State *L);
static int               child_getpid(lua_State *L);
static int               child_getrpid(lua_State *L);
static int               child_getstatus(lua_State *L);

/**
 * Stat functions:
 */
static int               luaopen_ev_stat(lua_State *L);
static int               create_stat_mt(lua_State *L);
static int               stat_new(lua_State* L);
static void              stat_cb(struct ev_loop* loop, ev_stat* sig, int revents);
static int               stat_stop(lua_State *L);
static int               stat_start(lua_State *L);
static int               stat_start(lua_State *L);
static int               stat_getdata(lua_State *L);

/* vi:set expandtab ts=4: */
