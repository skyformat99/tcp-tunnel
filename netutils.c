#define _GNU_SOURCE
#include "netutils.h"
#include "logutils.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#undef _GNU_SOURCE

#ifndef SO_ORIGINAL_DST
#define SO_ORIGINAL_DST 80
#endif

#ifndef IP6T_SO_ORIGINAL_DST
#define IP6T_SO_ORIGINAL_DST 80
#endif

/* setsockopt(TCP_NODELAY) */
void set_no_delay(int sockfd) {
    if (setsockopt(sockfd, SOL_TCP, TCP_NODELAY, &(int){1}, sizeof(int))) {
        LOGERR("[set_no_delay] setsockopt(%d, TCP_NODELAY): (%d) %s", sockfd, errno, errstring(errno));
        exit(errno);
    }
}

/* setsockopt(IPV6_V6ONLY) */
void set_ipv6_only(int sockfd) {
    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &(int){1}, sizeof(int))) {
        LOGERR("[set_ipv6_only] setsockopt(%d, IPV6_V6ONLY): (%d) %s", sockfd, errno, errstring(errno));
        exit(errno);
    }
}

/* setsockopt(SO_REUSEADDR) */
void set_reuse_addr(int sockfd) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        LOGERR("[set_reuse_addr] setsockopt(%d, SO_REUSEADDR): (%d) %s", sockfd, errno, errstring(errno));
        exit(errno);
    }
}

/* setsockopt(SO_REUSEPORT) */
void set_reuse_port(int sockfd) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int))) {
        LOGERR("[set_reuse_port] setsockopt(%d, SO_REUSEPORT): (%d) %s", sockfd, errno, errstring(errno));
        exit(errno);
    }
}

/* setsockopt(IP_TRANSPARENT) */
void set_transparent(int sockfd) {
    if (setsockopt(sockfd, SOL_IP, IP_TRANSPARENT, &(int){1}, sizeof(int))) {
        LOGERR("[set_transparent] setsockopt(%d, IP_TRANSPARENT): (%d) %s", sockfd, errno, errstring(errno));
        exit(errno);
    }
}

/* setsockopt(SO_MARK) */
void set_socket_mark(int sockfd, uint32_t mark) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_MARK, &mark, sizeof(uint32_t))) {
        LOGERR("[set_socket_mark] setsockopt(%d, SO_MARK): (%d) %s", sockfd, errno, errstring(errno));
        exit(errno);
    }
}

/* getsockopt(SO_ORIGINAL_DST) ipv4 */
bool get_origdstaddr4(int sockfd, skaddr4_t *dstaddr) {
    if (getsockopt(sockfd, SOL_IP, SO_ORIGINAL_DST, dstaddr, &(socklen_t){sizeof(skaddr4_t)})) {
        LOGERR("[get_origdstaddr4] getsockopt(%d, SO_ORIGINAL_DST): (%d) %s", sockfd, errno, errstring(errno));
        return false;
    }
    return true;
}

/* getsockopt(IP6T_SO_ORIGINAL_DST) ipv6 */
bool get_origdstaddr6(int sockfd, skaddr6_t *dstaddr) {
    if (getsockopt(sockfd, SOL_IPV6, IP6T_SO_ORIGINAL_DST, dstaddr, &(socklen_t){sizeof(skaddr6_t)})) {
        LOGERR("[get_origdstaddr6] getsockopt(%d, IP6T_SO_ORIGINAL_DST): (%d) %s", sockfd, errno, errstring(errno));
        return false;
    }
    return true;
}

/* create non-blocking tcp socket (ipv4) */
int new_tcp4_socket(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd < 0) {
        LOGERR("[new_tcp4_socket] socket(AF_INET, SOCK_STREAM): (%d) %s", errno, errstring(errno));
        exit(errno);
    }
    return sockfd;
}

/* create non-blocking tcp socket (ipv6) */
int new_tcp6_socket(void) {
    int sockfd = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd < 0) {
        LOGERR("[new_tcp6_socket] socket(AF_INET6, SOCK_STREAM): (%d) %s", errno, errstring(errno));
        exit(errno);
    }
    return sockfd;
}

/* create tcp socket use to connect (ipv4) */
int new_tcp4_connsock(uint32_t somark) {
    int sockfd = new_tcp4_socket();
    set_no_delay(sockfd);
    if (somark) set_socket_mark(sockfd, somark);
    return sockfd;
}

/* create tcp socket use to connect (ipv6) */
int new_tcp6_connsock(uint32_t somark) {
    int sockfd = new_tcp6_socket();
    set_no_delay(sockfd);
    if (somark) set_socket_mark(sockfd, somark);
    return sockfd;
}

/* create tcp socket use to listen (ipv4) */
int new_tcp4_bindsock(void) {
    int sockfd = new_tcp4_socket();
    set_reuse_addr(sockfd);
    set_reuse_port(sockfd);
    return sockfd;
}

/* create tcp socket use to listen (ipv6) */
int new_tcp6_bindsock(void) {
    int sockfd = new_tcp6_socket();
    set_ipv6_only(sockfd);
    set_reuse_addr(sockfd);
    set_reuse_port(sockfd);
    return sockfd;
}

/* create tcp socket use to tproxy-listen (ipv4) */
int new_tcp4_bindsock_tproxy(void) {
    int sockfd = new_tcp4_bindsock();
    set_transparent(sockfd);
    return sockfd;
}

/* create tcp socket use to tproxy-listen (ipv6) */
int new_tcp6_bindsock_tproxy(void) {
    int sockfd = new_tcp6_bindsock();
    set_transparent(sockfd);
    return sockfd;
}

/* build ipv4 socket address from ipstr and portno */
void build_ipv4_addr(skaddr4_t *addr, const char *ipstr, portno_t portno) {
    addr->sin_family = AF_INET;
    inet_pton(AF_INET, ipstr, &addr->sin_addr);
    addr->sin_port = htons(portno);
}

/* build ipv6 socket address from ipstr and portno */
void build_ipv6_addr(skaddr6_t *addr, const char *ipstr, portno_t portno) {
    addr->sin6_family = AF_INET6;
    inet_pton(AF_INET6, ipstr, &addr->sin6_addr);
    addr->sin6_port = htons(portno);
}

/* parse ipstr and portno from ipv4 socket address */
void parse_ipv4_addr(const skaddr4_t *addr, char *ipstr, portno_t *portno) {
    inet_ntop(AF_INET, &addr->sin_addr, ipstr, IP4STRLEN);
    *portno = ntohs(addr->sin_port);
}

/* parse ipstr and portno from ipv6 socket address */
void parse_ipv6_addr(const skaddr6_t *addr, char *ipstr, portno_t *portno) {
    inet_ntop(AF_INET6, &addr->sin6_addr, ipstr, IP6STRLEN);
    *portno = ntohs(addr->sin6_port);
}

/* AF_INET or AF_INET6 or -1(invalid ip string) */
int get_ipstr_family(const char *ipstr) {
    if (!ipstr) return -1;
    ipaddr6_t ipaddr; /* save ipv4/ipv6 addr */
    if (inet_pton(AF_INET, ipstr, &ipaddr) == 1) {
        return AF_INET;
    } else if (inet_pton(AF_INET6, ipstr, &ipaddr) == 1) {
        return AF_INET6;
    } else {
        return -1;
    }
}
