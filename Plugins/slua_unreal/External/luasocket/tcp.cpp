/*=========================================================================*\
* TCP object
* LuaSocket toolkit
\*=========================================================================*/
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#include "auxiliar.h"
#include "socket.h"
#include "inet.h"
#include "options.h"
#include "tcp.h"

namespace NS_SLUA {    

/*=========================================================================*\
* Internal function prototypes
\*=========================================================================*/
static int tcp_global_create(lua_State *L);
static int tcp_global_create6(lua_State *L);
static int tcp_global_connect(lua_State *L);
static int tcp_meth_connect(lua_State *L);
static int tcp_meth_listen(lua_State *L);
static int tcp_meth_getfamily(lua_State *L);
static int tcp_meth_bind(lua_State *L);
static int tcp_meth_send(lua_State *L);
static int tcp_meth_getstats(lua_State *L);
static int tcp_meth_setstats(lua_State *L);
static int tcp_meth_getsockname(lua_State *L);
static int tcp_meth_getpeername(lua_State *L);
static int tcp_meth_shutdown(lua_State *L);
static int tcp_meth_receive(lua_State *L);
static int tcp_meth_accept(lua_State *L);
static int tcp_meth_close(lua_State *L);
static int tcp_meth_getoption(lua_State *L);
static int tcp_meth_setoption(lua_State *L);
static int tcp_meth_settimeout(lua_State *L);
static int tcp_meth_getfd(lua_State *L);
static int tcp_meth_setfd(lua_State *L);
static int tcp_meth_dirty(lua_State *L);

/* tcp object methods */
static luaL_Reg tcp_methods[] = {
    {"__gc",        tcp_meth_close},
    {"__tostring",  auxiliar_tostring},
    {"accept",      tcp_meth_accept},
    {"bind",        tcp_meth_bind},
    {"close",       tcp_meth_close},
    {"connect",     tcp_meth_connect},
    {"dirty",       tcp_meth_dirty},
    {"getfamily",   tcp_meth_getfamily},
    {"getfd",       tcp_meth_getfd},
    {"getoption",   tcp_meth_getoption},
    {"getpeername", tcp_meth_getpeername},
    {"getsockname", tcp_meth_getsockname},
    {"getstats",    tcp_meth_getstats},
    {"setstats",    tcp_meth_setstats},
    {"listen",      tcp_meth_listen},
    {"receive",     tcp_meth_receive},
    {"send",        tcp_meth_send},
    {"setfd",       tcp_meth_setfd},
    {"setoption",   tcp_meth_setoption},
    {"setpeername", tcp_meth_connect},
    {"setsockname", tcp_meth_bind},
    {"settimeout",  tcp_meth_settimeout},
    {"shutdown",    tcp_meth_shutdown},
    {NULL,          NULL}
};

/* socket option handlers */
static t_opt tcp_optget[] = {
    {"keepalive",   opt_get_keepalive},
    {"reuseaddr",   opt_get_reuseaddr},
    {"tcp-nodelay", opt_get_tcp_nodelay},
    {"linger",      opt_get_linger},
    {"error",       opt_get_error},
    {NULL,          NULL}
};

static t_opt tcp_optset[] = {
    {"keepalive",   opt_set_keepalive},
    {"reuseaddr",   opt_set_reuseaddr},
    {"tcp-nodelay", opt_set_tcp_nodelay},
    {"ipv6-v6only", opt_set_ip6_v6only},
    {"linger",      opt_set_linger},
    {NULL,          NULL}
};

/* functions in library namespace */
static luaL_Reg tcp_func[] = {
    {"tcp", tcp_global_create},
    {"tcp6", tcp_global_create6},
    {"connect", tcp_global_connect},
    {NULL, NULL}
};

/*-------------------------------------------------------------------------*\
* Initializes module
\*-------------------------------------------------------------------------*/
int tcp_open(lua_State *L)
{
    /* create classes */
    auxiliar_newclass(L, "tcp{master}", tcp_methods);
    auxiliar_newclass(L, "tcp{client}", tcp_methods);
    auxiliar_newclass(L, "tcp{server}", tcp_methods);
    /* create class groups */
    auxiliar_add2group(L, "tcp{master}", "tcp{any}");
    auxiliar_add2group(L, "tcp{client}", "tcp{any}");
    auxiliar_add2group(L, "tcp{server}", "tcp{any}");
    /* define library functions */
#if LUA_VERSION_NUM > 501 && !defined(LUA_COMPAT_MODULE)
    luaL_setfuncs(L, tcp_func, 0);
#else
    luaL_openlib(L, NULL, tcp_func, 0);
#endif
    return 0;
}

/*=========================================================================*\
* Lua methods
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Just call buffered IO methods
\*-------------------------------------------------------------------------*/
static int tcp_meth_send(lua_State *L) {
    p_tcp tcp = (p_tcp) auxiliar_checkclass(L, "tcp{client}", 1);
    return buffer_meth_send(L, &tcp->buf);
}

static int tcp_meth_receive(lua_State *L) {
    p_tcp tcp = (p_tcp) auxiliar_checkclass(L, "tcp{client}", 1);
    return buffer_meth_receive(L, &tcp->buf);
}

static int tcp_meth_getstats(lua_State *L) {
    p_tcp tcp = (p_tcp) auxiliar_checkclass(L, "tcp{client}", 1);
    return buffer_meth_getstats(L, &tcp->buf);
}

static int tcp_meth_setstats(lua_State *L) {
    p_tcp tcp = (p_tcp) auxiliar_checkclass(L, "tcp{client}", 1);
    return buffer_meth_setstats(L, &tcp->buf);
}

/*-------------------------------------------------------------------------*\
* Just call option handler
\*-------------------------------------------------------------------------*/
static int tcp_meth_getoption(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
    return opt_meth_getoption(L, tcp_optget, &tcp->sock);
}

static int tcp_meth_setoption(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
    return opt_meth_setoption(L, tcp_optset, &tcp->sock);
}

/*-------------------------------------------------------------------------*\
* Select support methods
\*-------------------------------------------------------------------------*/
static int tcp_meth_getfd(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
    lua_pushnumber(L, (int) tcp->sock);
    return 1;
}

/* this is very dangerous, but can be handy for those that are brave enough */
static int tcp_meth_setfd(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
    tcp->sock = (t_socket) luaL_checknumber(L, 2);
    return 0;
}

static int tcp_meth_dirty(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
    lua_pushboolean(L, !buffer_isempty(&tcp->buf));
    return 1;
}

/*-------------------------------------------------------------------------*\
* Waits for and returns a client object attempting connection to the
* server object
\*-------------------------------------------------------------------------*/
static int tcp_meth_accept(lua_State *L)
{
    p_tcp server = (p_tcp) auxiliar_checkclass(L, "tcp{server}", 1);
    p_timeout tm = timeout_markstart(&server->tm);
    t_socket sock;
    const char *err = inet_tryaccept(&server->sock, server->family, &sock, tm);
    /* if successful, push client socket */
    if (err == NULL) {
        p_tcp clnt = (p_tcp) lua_newuserdata(L, sizeof(t_tcp));
        auxiliar_setclass(L, "tcp{client}", -1);
        /* initialize structure fields */
        memset(clnt, 0, sizeof(t_tcp));
        socket_setnonblocking(&sock);
        clnt->sock = sock;
        io_init(&clnt->io, (p_send) socket_send, (p_recv) socket_recv,
                (p_error) socket_ioerror, &clnt->sock);
        timeout_init(&clnt->tm, -1, -1);
        buffer_init(&clnt->buf, &clnt->io, &clnt->tm);
        clnt->family = server->family;
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, err);
        return 2;
    }
}

/*-------------------------------------------------------------------------*\
* Binds an object to an address
\*-------------------------------------------------------------------------*/
static int tcp_meth_bind(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkclass(L, "tcp{master}", 1);
    const char *address =  luaL_checkstring(L, 2);
    const char *port = luaL_checkstring(L, 3);
    const char *err;
    struct addrinfo bindhints;
    memset(&bindhints, 0, sizeof(bindhints));
    bindhints.ai_socktype = SOCK_STREAM;
    bindhints.ai_family = tcp->family;
    bindhints.ai_flags = AI_PASSIVE;
    err = inet_trybind(&tcp->sock, address, port, &bindhints);
    if (err) {
        lua_pushnil(L);
        lua_pushstring(L, err);
        return 2;
    }
    lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Turns a master tcp object into a client object.
\*-------------------------------------------------------------------------*/
static int tcp_meth_connect(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
    const char *address =  luaL_checkstring(L, 2);
    const char *port = luaL_checkstring(L, 3);
    struct addrinfo connecthints;
    const char *err;
    memset(&connecthints, 0, sizeof(connecthints));
    connecthints.ai_socktype = SOCK_STREAM;
    /* make sure we try to connect only to the same family */
    connecthints.ai_family = tcp->family;
    timeout_markstart(&tcp->tm);
    err = inet_tryconnect(&tcp->sock, &tcp->family, address, port, 
        &tcp->tm, &connecthints);
    /* have to set the class even if it failed due to non-blocking connects */
    auxiliar_setclass(L, "tcp{client}", 1);
    if (err) {
        lua_pushnil(L);
        lua_pushstring(L, err);
        return 2;
    }
    lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Closes socket used by object
\*-------------------------------------------------------------------------*/
static int tcp_meth_close(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
    socket_destroy(&tcp->sock);
    lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Returns family as string
\*-------------------------------------------------------------------------*/
static int tcp_meth_getfamily(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
    if (tcp->family == PF_INET6) {
        lua_pushliteral(L, "inet6");
        return 1;
    } else {
        lua_pushliteral(L, "inet4");
        return 1;
    }
}

/*-------------------------------------------------------------------------*\
* Puts the sockt in listen mode
\*-------------------------------------------------------------------------*/
static int tcp_meth_listen(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkclass(L, "tcp{master}", 1);
    int backlog = (int) luaL_optnumber(L, 2, 32);
    int err = socket_listen(&tcp->sock, backlog);
    if (err != IO_DONE) {
        lua_pushnil(L);
        lua_pushstring(L, socket_strerror(err));
        return 2;
    }
    /* turn master object into a server object */
    auxiliar_setclass(L, "tcp{server}", 1);
    lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Shuts the connection down partially
\*-------------------------------------------------------------------------*/
static int tcp_meth_shutdown(lua_State *L)
{
    /* SHUT_RD,  SHUT_WR,  SHUT_RDWR  have  the value 0, 1, 2, so we can use method index directly */
    static const char* methods[] = { "receive", "send", "both", NULL };
    p_tcp tcp = (p_tcp) auxiliar_checkclass(L, "tcp{client}", 1);
    int how = luaL_checkoption(L, 2, "both", methods);
    socket_shutdown(&tcp->sock, how);
    lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Just call inet methods
\*-------------------------------------------------------------------------*/
static int tcp_meth_getpeername(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
    return inet_meth_getpeername(L, &tcp->sock, tcp->family);
}

static int tcp_meth_getsockname(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
    return inet_meth_getsockname(L, &tcp->sock, tcp->family);
}

/*-------------------------------------------------------------------------*\
* Just call tm methods
\*-------------------------------------------------------------------------*/
static int tcp_meth_settimeout(lua_State *L)
{
    p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
    return timeout_meth_settimeout(L, &tcp->tm);
}

/*=========================================================================*\
* Library functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Creates a master tcp object
\*-------------------------------------------------------------------------*/
static int tcp_create(lua_State *L, int family) {
    t_socket sock;
    const char *err = inet_trycreate(&sock, family, SOCK_STREAM);
    /* try to allocate a system socket */
    if (!err) {
        /* allocate tcp object */
        p_tcp tcp = (p_tcp) lua_newuserdata(L, sizeof(t_tcp));
        memset(tcp, 0, sizeof(t_tcp));
        /* set its type as master object */
        auxiliar_setclass(L, "tcp{master}", -1);
        /* initialize remaining structure fields */
        socket_setnonblocking(&sock);
        if (family == PF_INET6) {
            int yes = 1;
            setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
                reinterpret_cast<const char *>(&yes), sizeof(yes));
        }
        tcp->sock = sock;
        io_init(&tcp->io, (p_send) socket_send, (p_recv) socket_recv,
                (p_error) socket_ioerror, &tcp->sock);
        timeout_init(&tcp->tm, -1, -1);
        buffer_init(&tcp->buf, &tcp->io, &tcp->tm);
        tcp->family = family;
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, err);
        return 2;
    }
}

static int tcp_global_create(lua_State *L) {
    return tcp_create(L, AF_INET);
}

static int tcp_global_create6(lua_State *L) {
    return tcp_create(L, AF_INET6);
}

#if 0
static const char *tryconnect6(const char *remoteaddr, const char *remoteserv,
    struct addrinfo *connecthints, p_tcp tcp) {
    struct addrinfo *iterator = NULL, *resolved = NULL;
    const char *err = NULL;
    /* try resolving */
    err = socket_gaistrerror(getaddrinfo(remoteaddr, remoteserv,
                connecthints, &resolved));
    if (err != NULL) {
        if (resolved) freeaddrinfo(resolved);
        return err;
    }
    /* iterate over all returned addresses trying to connect */
    for (iterator = resolved; iterator; iterator = iterator->ai_next) {
        p_timeout tm = timeout_markstart(&tcp->tm);
        /* create new socket if necessary. if there was no
         * bind, we need to create one for every new family
         * that shows up while iterating. if there was a
         * bind, all families will be the same and we will
         * not enter this branch. */
        if (tcp->family != iterator->ai_family) {
            socket_destroy(&tcp->sock);
            err = socket_strerror(socket_create(&tcp->sock,
                iterator->ai_family, iterator->ai_socktype,
                iterator->ai_protocol));
            if (err != NULL) {
                freeaddrinfo(resolved);
                return err;
            }
            tcp->family = iterator->ai_family;
            /* all sockets initially non-blocking */
            socket_setnonblocking(&tcp->sock);
        }
        /* finally try connecting to remote address */
        err = socket_strerror(socket_connect(&tcp->sock,
            (SA *) iterator->ai_addr,
            (socklen_t) iterator->ai_addrlen, tm));
        /* if success, break out of loop */
        if (err == NULL) break;
    }

    freeaddrinfo(resolved);
    /* here, if err is set, we failed */
    return err;
}
#endif

static int tcp_global_connect(lua_State *L) {
    const char *remoteaddr = luaL_checkstring(L, 1);
    const char *remoteserv = luaL_checkstring(L, 2);
    const char *localaddr  = luaL_optstring(L, 3, NULL);
    const char *localserv  = luaL_optstring(L, 4, "0");
    int family = inet_optfamily(L, 5, "unspec");
    p_tcp tcp = (p_tcp) lua_newuserdata(L, sizeof(t_tcp));
    struct addrinfo bindhints, connecthints;
    const char *err = NULL;
    /* initialize tcp structure */
    memset(tcp, 0, sizeof(t_tcp));
    io_init(&tcp->io, (p_send) socket_send, (p_recv) socket_recv,
            (p_error) socket_ioerror, &tcp->sock);
    timeout_init(&tcp->tm, -1, -1);
    buffer_init(&tcp->buf, &tcp->io, &tcp->tm);
    tcp->sock = SOCKET_INVALID;
    tcp->family = PF_UNSPEC;
    /* allow user to pick local address and port */
    memset(&bindhints, 0, sizeof(bindhints));
    bindhints.ai_socktype = SOCK_STREAM;
    bindhints.ai_family = family;
    bindhints.ai_flags = AI_PASSIVE;
    if (localaddr) {
        err = inet_trybind(&tcp->sock, localaddr, localserv, &bindhints);
        if (err) {
            lua_pushnil(L);
            lua_pushstring(L, err);
            return 2;
        }
        tcp->family = bindhints.ai_family;
    }
    /* try to connect to remote address and port */
    memset(&connecthints, 0, sizeof(connecthints));
    connecthints.ai_socktype = SOCK_STREAM;
    /* make sure we try to connect only to the same family */
    connecthints.ai_family = bindhints.ai_family;
    err = inet_tryconnect(&tcp->sock, &tcp->family, remoteaddr, remoteserv,
         &tcp->tm, &connecthints);
    if (err) {
        socket_destroy(&tcp->sock);
        lua_pushnil(L);
        lua_pushstring(L, err);
        return 2;
    }
    auxiliar_setclass(L, "tcp{client}", -1);
    return 1;
}

} // end NS_SLUA
