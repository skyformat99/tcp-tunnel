#ifndef TCPTUNNEL_PROTOCOL_H
#define TCPTUNNEL_PROTOCOL_H

#define _GNU_SOURCE
#include "netutils.h"
#undef _GNU_SOURCE

#define MSGTYPE_HEARTBEAT 0x00 /* tun-server heartbeat (on demand) */
#define MSGTYPE_CONNECT4  0x01 /* ipv4 connection request (async) */
#define MSGTYPE_CONNECT6  0x02 /* ipv6 connection request (async) */
#define MSGTYPE_ESTABLISH 0x03 /* connection established notice */
#define MSGTYPE_PAYLOAD   0x04 /* application layer payload */
#define MSGTYPE_CLOSE     0x05 /* connection close notice */
#define MSGTYPE_ERROR     0x06 /* an error occurred */

typedef struct {
    uint8_t type;
} __attribute__((packed)) msg_heartbeat_t;

typedef struct {
    uint8_t   type;
    uint64_t  sptr; /* source object pointer */
    ipaddr4_t addr; /* target ipv4 address */
    portno_t  port; /* target port number */
} __attribute__((packed)) msg_connect4_t;

typedef struct {
    uint8_t   type;
    uint64_t  sptr; /* source object pointer */
    ipaddr6_t addr; /* target ipv6 address */
    portno_t  port; /* target port number */
} __attribute__((packed)) msg_connect6_t;

typedef struct {
    uint8_t  type;
    uint64_t sptr; /* source object pointer */
    uint64_t dptr; /* target object pointer */
} __attribute__((packed)) msg_establish_t;

/* with payload */
typedef struct {
    uint8_t  type;
    uint64_t ptr; /* associated object pointer */
    uint32_t len; /* the length of the payload */
    uint8_t  data[]; /* payload (sizeof is zero) */
} __attribute__((packed)) msg_payload_t;

typedef struct {
    uint8_t  type;
    uint64_t ptr; /* associated object pointer */
} __attribute__((packed)) msg_close_t;

typedef struct {
    uint8_t  type;
    uint64_t ptr; /* associated object pointer */
} __attribute__((packed)) msg_error_t;

#endif
