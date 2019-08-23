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
static bool      g_verbose                = false;
static uint8_t   g_options                = OPTION_IP4 | OPTION_IP6;
static uint16_t  g_thread_num             = THREAD_NUMBER_DEFAULT;
static uint32_t  g_cli_bufsize            = TCPCLIENT_BUFSIZE_DEFAULT;
static uint32_t  g_tun_bufsize            = TCPTUNNEL_BUFSIZE_DEFAULT;
static uint8_t   g_ackwait_msec           = CLIENT_ACKWAITSEC_DEFAULT * 1000;

static char      g_bind_ipstr4[IP4STRLEN] = CLIENT_BINDADDR4_DEFAULT;
static char      g_bind_ipstr6[IP6STRLEN] = CLIENT_BINDADDR6_DEFAULT;
static portno_t  g_bind_portno            = CLIENT_BINDPORT_DEFAULT;
static skaddr4_t g_bind_skaddr4           = {0};
static skaddr4_t g_bind_skaddr6           = {0};

static bool      g_svr_isip4              = true;
static char      g_svr_ipstr[IP6STRLEN]   = SERVER_BINDADDR_DEFAULT;
static portno_t  g_svr_portno             = SERVER_BINDPORT_DEFAULT;
static skaddr6_t g_svr_skaddr             = {0};

int main() {
    LOGINF("[main] hello, world");
    return 0;
}
