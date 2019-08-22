#define _GNU_SOURCE
#include "logutils.h"
#undef _GNU_SOURCE

void set_line_buffering(void) {
    setvbuf(stdout, NULL, _IOLBF, 512);
}
