#define _GNU_SOURCE
#include "constant.h"
#include "hashset.h"
#include "logutils.h"
#include "netutils.h"
#include "protocol.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#undef _GNU_SOURCE

/* option flags */
enum {
    OPTION_IP4 = 0x01 << 0, /* listen for ipv4 address */
    OPTION_IP6 = 0x01 << 1, /* listen for ipv6 address */
    OPTION_NAT = 0x01 << 2, /* use redirect instead of tproxy */
};

/* if verbose logging */
#define IF_VERBOSE if (g_verbose)

/* thread local typedef */
typedef struct {
    uv_tcp_t     *tunnel; /* tunnel object */
    uv_connect_t *connreq; /* connect request */
    uv_timer_t   *timer; /* ack wait timer */
    hashset_t    *clients; /* all clients (set) */
    void         *buffer; /* tunnel recv buffer */
    uint32_t      nread; /* recv buffer data length */
    uint8_t       offset; /* recv buffer begin offset */
    bool          isready; /* tunnel connection is ready */
} loop_data_t;

/* client context typedef */
typedef struct {
    void       *clibuffer;
    uv_write_t *cliwrtreq;
    uv_write_t *tunwrtreq;
    uint64_t    peerptr;
    uint8_t     status;
} client_data_t;

/* function declaration */
static void* run_event_loop(void *arg);

static void tunnel_try_connect(uv_loop_t *loop, bool is_sleep);
static void tunnel_connect_cb(uv_connect_t *connreq, int status);
static void tunnel_alloc_cb(uv_handle_t *tunnel, size_t sugsize, uv_buf_t *uvbuf);
static void tunnel_read_cb(uv_stream_t *tunnel, ssize_t nread, const uv_buf_t *uvbuf);
static void tunnel_handle_data(uv_stream_t *tunnel, uint8_t *buffer, uint32_t length);
static void tunnel_write_cb(uv_write_t *writereq, int status);
static void tunnel_timer_cb(uv_timer_t *acktimer);
static void tunnel_close_cb(uv_handle_t *tunnel);

static void client_accept_cb(uv_stream_t *listener, int status);
static void client_alloc_cb(uv_handle_t *client, size_t sugsize, uv_buf_t *uvbuf);
static void client_read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *uvbuf);
static void client_write_cb(uv_write_t *writereq, int status);
static void client_close_cb(uv_handle_t *client);
static void client_elemfree_cb(void *elem);

/* static global variables */
static bool      g_verbose                = false; /* verbose mode */
static uint8_t   g_options                = OPTION_IP4 | OPTION_IP6;
static uint16_t  g_thread_num             = THREAD_NUMBER_DEFAULT;
static uint32_t  g_cli_bufsize            = TCPCLIENT_BUFSIZE_DEFAULT;
static uint32_t  g_tun_bufsize            = TCPTUNNEL_BUFSIZE_DEFAULT;
static uint8_t   g_ackwait_sec            = CLIENT_ACKWAITSEC_DEFAULT;
static uint32_t  g_socket_mark            = 0; /* 0 means disabled */

static char      g_bind_ipstr4[IP4STRLEN] = CLIENT_BINDADDR4_DEFAULT;
static char      g_bind_ipstr6[IP6STRLEN] = CLIENT_BINDADDR6_DEFAULT;
static portno_t  g_bind_portno            = CLIENT_BINDPORT_DEFAULT;
static skaddr4_t g_bind_skaddr4           = {0};
static skaddr6_t g_bind_skaddr6           = {0};

static char      g_svr_ipstr[IP6STRLEN]   = SERVER_BINDADDR_DEFAULT;
static portno_t  g_svr_portno             = SERVER_BINDPORT_DEFAULT;
static skaddr6_t g_svr_skaddr             = {0};

static void print_command_help(void) {
    printf("usage: tun-client <options...>. the existing options are as follows:\n"
           " -s, --server-addr <addr>           server ip or host, default: 127.0.0.1\n"
           " -p, --server-port <port>           server port number, default: 61080\n"
           " -b, --listen-addr4 <addr>          listen address (ipv4), default: 127.0.0.1\n" 
           " -B, --listen-addr6 <addr>          listen address (ipv6), default: ::1\n"
           " -l, --listen-port <port>           listen port number, default: 61080\n"
           " -j, --thread-num <num>             number of worker threads, default: 2\n"
           " -a, --ack-timeout <sec>            tcp tunnel ack timeout sec, default: 5\n"
           " -z, --cli-bufsize <size>           client recv buffer size, default: 8192\n"
           " -Z, --tun-bufsize <size>           tunnel recv buffer size, default: 32768\n"
           " -m, --set-mark <mark>              use mark to tag the packets going out\n"
           " -R, --redirect                     use redirect(nat) instead of tproxy\n"
           " -4, --ipv4-only                    listen ipv4 only, aka: disable ipv6\n"
           " -6, --ipv6-only                    listen ipv6 only, aka: disable ipv4\n"
           " -v, --verbose                      verbose mode, affect performance\n"
           " -V, --version                      print the version number and exit\n"
           " -h, --help                         print the help information and exit\n"
    );
}

static void parse_command_args(int argc, char *argv[]) {
    const char *optstr = ":s:p:b:B:l:j:a:z:Z:m:R46vVh";
    const struct option options[] = {
        {"server-addr",   required_argument, NULL, 's'},
        {"server-port",   required_argument, NULL, 'p'},
        {"listen-addr4",  required_argument, NULL, 'b'},
        {"listen-addr6",  required_argument, NULL, 'B'},
        {"listen-port",   required_argument, NULL, 'l'},
        {"thread-num",    required_argument, NULL, 'j'},
        {"ack-timeout",   required_argument, NULL, 'a'},
        {"cli-bufsize",   required_argument, NULL, 'z'},
        {"tun-bufsize",   required_argument, NULL, 'Z'},
        {"set-mark",      required_argument, NULL, 'm'},
        {"redirect",      no_argument,       NULL, 'R'},
        {"ipv4-only",     no_argument,       NULL, '4'},
        {"ipv6-only",     no_argument,       NULL, '6'},
        {"verbose",       no_argument,       NULL, 'v'},
        {"version",       no_argument,       NULL, 'V'},
        {"help",          no_argument,       NULL, 'h'},
        {NULL,            0,                 NULL,   0},
    };

    opterr = 0;
    int optindex = -1;
    int shortopt = -1;
    char *opt_server_addr = NULL;

    while ((shortopt = getopt_long(argc, argv, optstr, options, &optindex)) != -1) {
        switch (shortopt) {
            case 's':
                opt_server_addr = optarg;
                break;
            case 'p':
                if (strlen(optarg) + 1 > PORTSTRLEN) {
                    printf("[parse_command_args] port number max length is 5: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                g_svr_portno = strtol(optarg, NULL, 10);
                if (g_svr_portno == 0) {
                    printf("[parse_command_args] invalid server port number: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                break;
            case 'b':
                if (strlen(optarg) + 1 > IP4STRLEN) {
                    printf("[parse_command_args] ipv4 address max length is 15: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                if (get_ipstr_family(optarg) != AF_INET) {
                    printf("[parse_command_args] invalid listen ipv4 address: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                strcpy(g_bind_ipstr4, optarg);
                break;
            case 'B':
                if (strlen(optarg) + 1 > IP6STRLEN) {
                    printf("[parse_command_args] ipv6 address max length is 45: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                if (get_ipstr_family(optarg) != AF_INET6) {
                    printf("[parse_command_args] invalid listen ipv6 address: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                strcpy(g_bind_ipstr6, optarg);
                break;
            case 'l':
                if (strlen(optarg) + 1 > PORTSTRLEN) {
                    printf("[parse_command_args] port number max length is 5: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                g_bind_portno = strtol(optarg, NULL, 10);
                if (g_bind_portno == 0) {
                    printf("[parse_command_args] invalid listen port number: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                break;
            case 'j':
                g_thread_num = strtol(optarg, NULL, 10);
                if (g_thread_num == 0) {
                    printf("[parse_command_args] invalid number of worker threads: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                break;
            case 'a':
                g_ackwait_sec = strtol(optarg, NULL, 10);
                if (g_ackwait_sec == 0) {
                    printf("[parse_command_args] invalid tunnel ack timeout value: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                break;
            case 'z':
                g_cli_bufsize = strtol(optarg, NULL, 10);
                if (g_cli_bufsize < 1024) {
                    printf("[parse_command_args] buffer size is at least 1024 bytes: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                break;
            case 'Z':
                g_tun_bufsize = strtol(optarg, NULL, 10);
                if (g_tun_bufsize < 1024) {
                    printf("[parse_command_args] buffer size is at least 1024 bytes: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                break;
            case 'm':
                g_socket_mark = strtol(optarg, NULL, 10);
                if (g_socket_mark == 0) {
                    printf("[parse_command_args] invalid outgoing socket mark value: %s\n", optarg);
                    goto PRINT_HELP_AND_EXIT;
                }
                break;
            case 'R':
                g_options |= OPTION_NAT;
                break;
            case '4':
                g_options &= ~OPTION_IP6;
                break;
            case '6':
                g_options &= ~OPTION_IP4;
                break;
            case 'v':
                g_verbose = true;
                break;
            case 'V':
                printf(CLIENT_VERSION_STRING"\n");
                exit(0);
            case 'h':
                print_command_help();
                exit(0);
            case ':':
                printf("[parse_command_args] missing optarg: '%s'\n", argv[optind - 1]);
                goto PRINT_HELP_AND_EXIT;
            case '?':
                if (optopt) {
                    printf("[parse_command_args] unknown option: '-%c'\n", optopt);
                } else {
                    char *longopt = argv[optind - 1];
                    char *equalsign = strchr(longopt, '=');
                    if (equalsign) *equalsign = 0;
                    printf("[parse_command_args] unknown option: '%s'\n", longopt);
                }
                goto PRINT_HELP_AND_EXIT;
        }
    }

    if (!(g_options & OPTION_IP4) && !(g_options & OPTION_IP6)) {
        printf("[parse_command_args] both ipv4 and ipv6 are disabled, nothing to do\n");
        goto PRINT_HELP_AND_EXIT;
    }

    if (g_options & OPTION_NAT) {
        strcpy(g_bind_ipstr4, IP4ADDRSTR_WILDCARD);
        strcpy(g_bind_ipstr6, IP6ADDRSTR_WILDCARD);
    }
    build_ipv4_addr(&g_bind_skaddr4, g_bind_ipstr4, g_bind_portno);
    build_ipv6_addr(&g_bind_skaddr6, g_bind_ipstr6, g_bind_portno);

    if (opt_server_addr) {
        build_addr_byhostname((void *)&g_svr_skaddr, g_svr_ipstr, opt_server_addr, g_svr_portno);
    } else {
        build_ipv4_addr((void *)&g_svr_skaddr, g_svr_ipstr, g_svr_portno);
    }
    return;

PRINT_HELP_AND_EXIT:
    print_command_help();
    exit(1);
}

int main(int argc, char *argv[]) {
    parse_command_args(argc, argv);

    LOGINF("[main] server address: %s#%hu", g_svr_ipstr, g_svr_portno);
    if (g_options & OPTION_IP4) LOGINF("[main] listen address: %s#%hu", g_bind_ipstr4, g_bind_portno);
    if (g_options & OPTION_IP6) LOGINF("[main] listen address: %s#%hu", g_bind_ipstr6, g_bind_portno);
    LOGINF("[main] number of threads: %hu", g_thread_num);
    LOGINF("[main] tunnel ack timeout: %hhu", g_ackwait_sec);
    LOGINF("[main] client buffer size: %u", g_cli_bufsize);
    LOGINF("[main] tunnel buffer size: %u", g_tun_bufsize);
    if (g_socket_mark) LOGINF("[main] outgoing packet mark: %u", g_socket_mark);
    if (g_options & OPTION_NAT) LOGINF("[main] use redirect instead of tproxy");
    IF_VERBOSE LOGINF("[main] verbose mode, affect performance");

    for (int i = 0; i < g_thread_num - 1; ++i) {
        if (pthread_create(&(pthread_t){0}, NULL, run_event_loop, NULL)) {
            LOGERR("[main] failed to create thread: (%d) %s", errno, errstring(errno));
            exit(errno);
        }
    }
    run_event_loop(NULL);

    return 0;
}

static void* run_event_loop(void *arg __attribute__((unused))) {
    uv_loop_t *loop = &(uv_loop_t){0};
    uv_loop_init(loop);

    loop_data_t *loop_data = &(loop_data_t){0};
    loop->data = loop_data;

    if (g_options & OPTION_IP4) {
        uv_tcp_t *listener = malloc(sizeof(uv_tcp_t));
        listener->data = (void *)OPTION_IP4;
        uv_tcp_init(loop, listener);
        uv_tcp_open(listener, g_options & OPTION_NAT ? new_tcp4_bindsock() : new_tcp4_bindsock_tproxy());
        int retval = uv_tcp_bind(listener, (void *)&g_bind_skaddr4, 0);
        if (retval < 0) {
            LOGERR("[run_event_loop] failed to bind address for tcp4 socket: (%d) %s", -retval, uv_strerror(retval));
            exit(-retval);
        }
        retval = uv_listen((void *)listener, SOMAXCONN, client_accept_cb);
        if (retval < 0) {
            LOGERR("[run_event_loop] failed to listen address for tcp4 socket: (%d) %s", -retval, uv_strerror(retval));
            exit(-retval);
        }
    }

    if (g_options & OPTION_IP6) {
        uv_tcp_t *listener = malloc(sizeof(uv_tcp_t));
        listener->data = (void *)OPTION_IP6;
        uv_tcp_init(loop, listener);
        uv_tcp_open(listener, g_options & OPTION_NAT ? new_tcp6_bindsock() : new_tcp6_bindsock_tproxy());
        int retval = uv_tcp_bind(listener, (void *)&g_bind_skaddr6, 0);
        if (retval < 0) {
            LOGERR("[run_event_loop] failed to bind address for tcp6 socket: (%d) %s", -retval, uv_strerror(retval));
            exit(-retval);
        }
        retval = uv_listen((void *)listener, SOMAXCONN, client_accept_cb);
        if (retval < 0) {
            LOGERR("[run_event_loop] failed to listen address for tcp6 socket: (%d) %s", -retval, uv_strerror(retval));
            exit(-retval);
        }
    }

    loop_data->tunnel = malloc(sizeof(uv_tcp_t));
    loop_data->connreq = malloc(sizeof(uv_connect_t));
    loop_data->timer = malloc(sizeof(uv_timer_t));
    loop_data->clients = hashset_new();
    loop_data->buffer = malloc(g_tun_bufsize);
    loop_data->nread = 0;
    loop_data->offset = 0;
    loop_data->isready = false;

    uv_timer_t *acktimer = loop_data->timer;
    uv_timer_init(loop, acktimer);
    tunnel_try_connect(loop, false);

    uv_run(loop, UV_RUN_DEFAULT);
    return NULL;
}

static void tunnel_try_connect(uv_loop_t *loop, bool is_sleep) {
    if (is_sleep) sleep(1);
    loop_data_t *loop_data = loop->data;

    uv_timer_stop(loop_data->timer);
    hashset_clear(loop_data->clients, client_elemfree_cb);
    loop_data->nread = 0;
    loop_data->offset = 0;
    loop_data->isready = false;

    uv_tcp_t *tunnel = loop_data->tunnel;
    uv_tcp_init(loop, tunnel);
    uv_tcp_nodelay(tunnel, 1);

    IF_VERBOSE LOGINF("[tunnel_try_connect] try to connect to tun-server...");

    int retval;
    while ((retval = uv_tcp_connect(loop_data->connreq, tunnel, (void *)&g_svr_skaddr, tunnel_connect_cb)) < 0) {
        LOGERR("[tunnel_try_connect] failed to connect to tun-server: (%d) %s", -retval, uv_strerror(retval));
        sleep(1);
    }

    if (g_socket_mark) {
        int sockfd = -1;
        uv_fileno((void *)tunnel, &sockfd);
        set_socket_mark(sockfd, g_socket_mark);
    }
}

static void tunnel_connect_cb(uv_connect_t *connreq, int status) {
    if (status < 0) {
        LOGERR("[tunnel_connect_cb] failed to connect to tun-server: (%d) %s", -status, uv_strerror(status));
        uv_close((void *)connreq->handle, NULL); /* don't need close_cb */
        tunnel_try_connect(connreq->handle->loop, true);
        return;
    }

    IF_VERBOSE LOGINF("[tunnel_connect_cb] successfully connected to tun-server");

    uv_stream_t *tunnel = connreq->handle;
    uv_read_start(tunnel, tunnel_alloc_cb, tunnel_read_cb);

    loop_data_t *loop_data = tunnel->loop->data;
    loop_data->isready = true;
}

static void tunnel_alloc_cb(uv_handle_t *tunnel, size_t sugsize __attribute__((unused)), uv_buf_t *uvbuf) {
    loop_data_t *loop_data = tunnel->loop->data;
    uvbuf->base = loop_data->buffer + loop_data->offset;
    uvbuf->len = g_tun_bufsize - loop_data->offset;
}

static void tunnel_read_cb(uv_stream_t *tunnel, ssize_t nread, const uv_buf_t *uvbuf) {
    if (nread == 0) return;

    if (nread < 0) {
        if (nread != UV_EOF) LOGERR("[tunnel_read_cb] failed to read data from socket: (%zd) %s", -nread, uv_strerror(nread));
        uv_close((void *)tunnel, NULL); /* don't need close_cb */
        tunnel_try_connect(tunnel->loop, true);
        return;
    }

    loop_data_t *loop_data = tunnel->loop->data;
    tunnel_handle_data(tunnel, loop_data->buffer, nread);
}

static void tunnel_handle_data(uv_stream_t *tunnel, uint8_t *buffer, uint32_t length) {
    // TODO
}

static void tunnel_write_cb(uv_write_t *writereq, int status) {
    // TODO
}

static void tunnel_timer_cb(uv_timer_t *acktimer) {
    // TODO
}

static void tunnel_close_cb(uv_handle_t *tunnel) {
    // TODO
}

static void client_accept_cb(uv_stream_t *listener, int status) {
    // TODO
}

static void client_alloc_cb(uv_handle_t *client, size_t sugsize, uv_buf_t *uvbuf) {
    // TODO
}

static void client_read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *uvbuf) {
    // TODO
}

static void client_write_cb(uv_write_t *writereq, int status) {
    // TODO
}

static void client_close_cb(uv_handle_t *client) {
    // TODO
}

static void client_elemfree_cb(void *elem) {
    // TODO
}
