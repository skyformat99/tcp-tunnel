#ifndef TCPTUN_PROTOCOL_H
#define TCPTUN_PROTOCOL_H

#include <stdint.h>

#define MSGTYPE_SVR_ALIVE  0x00
#define MSGTYPE_TCP_CONN4  0x01
#define MSGTYPE_TCP_CONN6  0x02
#define MSGTYPE_TCP_ESTAB  0x03
#define MSGTYPE_TCP_DATA   0x04
#define MSGTYPE_TCP_CLOSE  0x05
#define MSGTYPE_TCP_ERROR  0x06
#define MSGTYPE_UDP_DGRAM4 0x07
#define MSGTYPE_UDP_DGRAM6 0x08

typedef struct {
    uint8_t msg_type;
} __attribute__((packed)) msg_svr_alive_t;

typedef struct {
    uint8_t  msg_type;
    uint16_t src_ctxid;
    uint32_t dst_ipaddr;
    uint16_t dst_portno;
} __attribute__((packed)) msg_tcp_conn4_t;

typedef struct {
    uint8_t  msg_type;
    uint16_t src_ctxid;
    uint8_t  dst_ipaddr[16];
    uint16_t dst_portno;
} __attribute__((packed)) msg_tcp_conn6_t;

typedef struct {
    uint8_t  msg_type;
    uint16_t src_ctxid;
    uint16_t dst_ctxid;
} __attribute__((packed)) msg_tcp_estab_t;

typedef struct {
    uint8_t  msg_type;
    uint16_t peer_ctxid;
    uint16_t data_length;
    uint8_t  data_baseptr[];
} __attribute__((packed)) msg_tcp_data_t;

typedef struct {
    uint8_t  msg_type;
    uint16_t peer_ctxid;
} __attribute__((packed)) msg_tcp_close_t;

typedef struct {
    uint8_t  msg_type;
    uint16_t peer_ctxid;
    int32_t  peer_errno;
} __attribute__((packed)) msg_tcp_error_t;

typedef struct {
    uint8_t  msg_type;
    uint32_t src_ipaddr;
    uint16_t src_portno;
    uint32_t dst_ipaddr;
    uint16_t dst_portno;
    uint16_t data_length;
    uint8_t  data_baseptr[];
} __attribute__((packed)) msg_udp_dgram4_t;

typedef struct {
    uint8_t  msg_type;
    uint8_t  src_ipaddr[16];
    uint16_t src_portno;
    uint8_t  dst_ipaddr[16];
    uint16_t dst_portno;
    uint16_t data_length;
    uint8_t  data_baseptr[];
} __attribute__((packed)) msg_udp_dgram6_t;

#endif
