#ifndef TCPTUNNEL_NETUTILS_H
#define TCPTUNNEL_NETUTILS_H

#define _GNU_SOURCE
#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <uv.h> /* uv_strerror */
#undef _GNU_SOURCE

/* ipaddr binary len */
#define IP4BINLEN 4
#define IP6BINLEN 16

/* ipaddr string len */
#define IP4STRLEN INET_ADDRSTRLEN
#define IP6STRLEN INET6_ADDRSTRLEN

/* portno string len */
#define PORTSTRLEN 6

/* ip address typedef */
typedef uint32_t ipaddr4_t;
typedef uint8_t  ipaddr6_t[16];

/* port number typedef */
typedef uint16_t portno_t;

/* sockaddr type alias */
typedef struct sockaddr     skaddr_t;
typedef struct sockaddr_in  skaddr4_t;
typedef struct sockaddr_in6 skaddr6_t;

/* setsockopt(TCP_NODELAY) */
void set_no_delay(int sockfd);

/* setsockopt(IPV6_V6ONLY) */
void set_ipv6_only(int sockfd);

/* setsockopt(SO_REUSEADDR) */
void set_reuse_addr(int sockfd);

/* setsockopt(SO_REUSEPORT) */
void set_reuse_port(int sockfd);

/* setsockopt(IP_TRANSPARENT) */
void set_transparent(int sockfd);

/* setsockopt(SO_MARK) */
void set_socket_mark(int sockfd, uint32_t mark);

/* getsockopt(SO_ORIGINAL_DST) ipv4 */
bool get_origdstaddr4(int sockfd, skaddr4_t *dstaddr);

/* getsockopt(IP6T_SO_ORIGINAL_DST) ipv6 */
bool get_origdstaddr6(int sockfd, skaddr6_t *dstaddr);

/* create non-blocking tcp socket (ipv4) */
int new_tcp4_socket(void);

/* create non-blocking tcp socket (ipv6) */
int new_tcp6_socket(void);

/* create tcp socket use to connect (ipv4) */
int new_tcp4_connsock(uint32_t somark);

/* create tcp socket use to connect (ipv6) */
int new_tcp6_connsock(uint32_t somark);

/* create tcp socket use to listen (ipv4) */
int new_tcp4_bindsock(void);

/* create tcp socket use to listen (ipv6) */
int new_tcp6_bindsock(void);

/* create tcp socket use to tproxy-listen (ipv4) */
int new_tcp4_bindsock_tproxy(void);

/* create tcp socket use to tproxy-listen (ipv6) */
int new_tcp6_bindsock_tproxy(void);

/* build ipv4 socket address from ipstr and portno */
void build_ipv4_addr(skaddr4_t *addr, const char *ipstr, portno_t portno);

/* build ipv6 socket address from ipstr and portno */
void build_ipv6_addr(skaddr6_t *addr, const char *ipstr, portno_t portno);

/* parse ipstr and portno from ipv4 socket address */
void parse_ipv4_addr(const skaddr4_t *addr, char *ipstr, portno_t *portno);

/* parse ipstr and portno from ipv6 socket address */
void parse_ipv6_addr(const skaddr6_t *addr, char *ipstr, portno_t *portno);

/* AF_INET or AF_INET6 or -1(invalid ip string) */
int get_ipstr_family(const char *ipstr);

/* uv_close() wrapper function */
void uv_tcpclose(uv_tcp_t *handle, uv_close_cb close_cb);

/* uv_tcp_close_reset() wrapper function */
void uv_tcpreset(uv_tcp_t *handle, uv_close_cb close_cb);

/* strerror thread safe version (libuv) */
#define errstring(errnum) uv_strerror(-(errnum))

#endif
