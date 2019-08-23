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

int main() {
    LOGINF("[main] hello, world");
    return 0;
}
