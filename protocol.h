#ifndef TCPTUNNEL_PROTOCOL_H
#define TCPTUNNEL_PROTOCOL_H

#define _GNU_SOURCE
#include "netutils.h"
#undef _GNU_SOURCE

#define MSGTYPE_HEARTBEAT 0x00
#define MSGTYPE_CONNECT4  0x01
#define MSGTYPE_CONNECT6  0x02
#define MSGTYPE_ESTABLISH 0x03
#define MSGTYPE_PAYLOAD   0x04
#define MSGTYPE_CLOSE     0x05
#define MSGTYPE_ERROR     0x06

typedef struct {
    uint8_t type;
} __attribute__((packed)) msg_heartbeat_t;

typedef struct {
    uint8_t   type;
    uint64_t  sptr;
    ipaddr4_t addr;
    portno_t  port;
} __attribute__((packed)) msg_connect4_t;

typedef struct {
    uint8_t   type;
    uint64_t  sptr;
    ipaddr6_t addr;
    portno_t  port;
} __attribute__((packed)) msg_connect6_t;

typedef struct {
    uint8_t  type;
    uint64_t sptr;
    uint64_t dptr;
} __attribute__((packed)) msg_establish_t;

/* with payload */
typedef struct {
    uint8_t  type;
    uint64_t ptr;
    uint32_t len;
    uint8_t  data[]; // sizeof = 0
} __attribute__((packed)) msg_payload_t;

typedef struct {
    uint8_t  type;
    uint64_t ptr;
} __attribute__((packed)) msg_close_t;

typedef struct {
    uint8_t  type;
    uint64_t ptr;
} __attribute__((packed)) msg_error_t;

#endif
