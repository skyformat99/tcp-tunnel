#ifndef TCPTUNNEL_CONSTANT_H
#define TCPTUNNEL_CONSTANT_H

/* default number of threads */
#define CLIENT_THREADNUM_DEFAULT 2
#define SERVER_THREADNUM_DEFAULT 2

/* default bufsize of tcp stream */
#define TCPSTREAM_BUFSIZE_MINIMUM 1024
#define CLTSTREAM_BUFSIZE_DEFAULT 8192
#define TUNSTREAM_BUFSIZE_DEFAULT 32768

/* default duration of ack timer */
#define CLIENT_ACKWAITSEC_DEFAULT 5
#define SERVER_ACKDELAYSEC_DEFAULT 2

/* string constant of ip address */
#define IP4ADDRSTR_LOOPBACK "127.0.0.1"
#define IP4ADDRSTR_WILDCARD "0.0.0.0"
#define IP6ADDRSTR_LOOPBACK "::1"
#define IP6ADDRSTR_WILDCARD "::"

/* default bind address of tunclient */
#define CLIENT_BINDADDR4_DEFAULT IP4ADDRSTR_LOOPBACK
#define CLIENT_BINDADDR6_DEFAULT IP6ADDRSTR_LOOPBACK
#define CLIENT_BINDPORT_DEFAULT 61080

/* default bind address of tunserver */
#define SERVER_BINDADDR_DEFAULT IP4ADDRSTR_LOOPBACK
#define SERVER_BINDPORT_DEFAULT 61080

/* string constant of tcptunnel version */
#define CLIENT_VERSION_STRING "tun-client v1.0.0"
#define SERVER_VERSION_STRING "tun-server v1.0.0"

/* several states of tcpclient connection */
enum {
    STATE_WAITREAD = 0, /* waiting for read_cb callback trigger */
    STATE_RECVDATA = 1, /* received payload from local, queued */
    STATE_RECVEOF  = 2, /* received EOF/FIN from local, queued */
    STATE_RECVERR  = 3, /* received ERROR from local, queued */
    STATE_ERRHANG  = 4, /* send err_msg to peer after queued */
};

#endif
