#ifndef TCPTUNNEL_CONSTANT_H
#define TCPTUNNEL_CONSTANT_H

#define THREAD_NUMBER_DEFAULT 2

#define TCPCLIENT_BUFSIZE_DEFAULT 8192
#define TCPTUNNEL_BUFSIZE_DEFAULT 32768

#define CLIENT_ACKWAITSEC_DEFAULT  5
#define SERVER_ACKDELAYSEC_DEFAULT 2

#define IP4ADDRSTR_LOOPBACK "127.0.0.1"
#define IP4ADDRSTR_WILDCARD "0.0.0.0"
#define IP6ADDRSTR_LOOPBACK "::1"
#define IP6ADDRSTR_WILDCARD "::"

#define CLIENT_BINDADDR4_DEFAULT IP4ADDRSTR_LOOPBACK
#define CLIENT_BINDADDR6_DEFAULT IP6ADDRSTR_LOOPBACK
#define CLIENT_BINDPORT_DEFAULT 61080

#define SERVER_BINDADDR_DEFAULT IP4ADDRSTR_LOOPBACK
#define SERVER_BINDPORT_DEFAULT 61080

#define CLIENT_VERSION_STRING "tun-client v1.0"
#define SERVER_VERSION_STRING "tun-server v1.0"

#endif
