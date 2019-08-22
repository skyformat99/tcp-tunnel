#define _GNU_SOURCE
#include "netutils.h"
#include "logutils.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
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

/* setsockopt(IPV6_V6ONLY) */
void set_ipv6_only(int sockfd);

/* setsockopt(SO_REUSEADDR) */
void set_reuse_addr(int sockfd);

/* setsockopt(SO_REUSEPORT) */
void set_reuse_port(int sockfd);

/* setsockopt(IP_TRANSPARENT) */
void set_transparent(int sockfd);

/* getsockopt(SO_ORIGINAL_DST) ipv4 */
bool get_origdstaddr4(int sockfd, skaddr4_t *dstaddr);

/* getsockopt(IP6T_SO_ORIGINAL_DST) ipv6 */
bool get_origdstaddr6(int sockfd, skaddr6_t *dstaddr);

/* create non-blocking tcp socket (ipv4) */
int new_tcp4_socket(void);

/* create non-blocking tcp socket (ipv6) */
int new_tcp6_socket(void);

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

/* build socket address by hostname (getaddrinfo) */
void build_addr_byhostname(skaddr_t *addr, char *ipstr, const char *hostname, portno_t portno);

/* AF_INET or AF_INET6 or -1(invalid ip string) */
int get_ipstr_family(const char *ipstr);

/* ignore SIGPIPE signal */
void ignore_sigpipe(void);
