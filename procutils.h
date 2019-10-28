#ifndef TCPTUNNEL_PROCUTILS_H
#define TCPTUNNEL_PROCUTILS_H

#define _GNU_SOURCE
#include <stddef.h>
#undef _GNU_SOURCE

/* ignore SIGPIPE signal by default */
void ignore_sigpipe(void) __attribute__((constructor));

/* set line buffering for stdout stream */
void set_line_buffering(void) __attribute__((constructor));

/* set nofile limit, maybe need root */
void set_nofile_limit(size_t nofile);

/* switch to the given user to run */
void run_as_user(const char *username, char *const argv[]);

#endif
