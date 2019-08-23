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

int main() {
    LOGINF("[main] hello, world");
    return 0;
}
