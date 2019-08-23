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
    LOGINF("[main] hello, world");
    return 0;
}
