#define _GNU_SOURCE
#include "procutils.h"
#include "logutils.h"
#include "netutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <linux/limits.h>
#undef _GNU_SOURCE

/* ignore SIGPIPE signal by default */
void ignore_sigpipe(void) {
    signal(SIGPIPE, SIG_IGN);
}

/* set line buffering for stdout stream */
void set_line_buffering(void) {
    setvbuf(stdout, NULL, _IOLBF, 256);
}

/* set nofile limit, maybe need root */
void set_nofile_limit(size_t nofile) {
    if (setrlimit(RLIMIT_NOFILE, &(struct rlimit){nofile, nofile}) < 0) {
        LOGERR("[set_nofile_limit] setrlimit(nofile, %zu): (%d) %s", nofile, errno, errstring(errno));
        exit(errno);
    }
}

/* suppress the warning of openwrt */
int initgroups(const char *user, gid_t group);

/* switch to the given user to run */
void run_as_user(const char *username, char *const argv[]) {
    if (geteuid() != 0) return; /* ignore if current user is not root */

    struct passwd *userinfo = getpwnam(username);
    if (!userinfo) {
        LOGERR("[run_as_user] the given user does not exist: %s", username);
        exit(1);
    }

    if (userinfo->pw_uid == 0) return; /* ignore if target user is root */

    if (setgid(userinfo->pw_gid) < 0) {
        LOGERR("[run_as_user] failed to change group_id of user '%s': (%d) %s", userinfo->pw_name, errno, errstring(errno));
        exit(errno);
    }

    if (initgroups(userinfo->pw_name, userinfo->pw_gid) < 0) {
        LOGERR("[run_as_user] failed to call initgroups() of user '%s': (%d) %s", userinfo->pw_name, errno, errstring(errno));
        exit(errno);
    }

    if (setuid(userinfo->pw_uid) < 0) {
        LOGERR("[run_as_user] failed to change user_id of user '%s': (%d) %s", userinfo->pw_name, errno, errstring(errno));
        exit(errno);
    }

    static char exec_file_abspath[PATH_MAX] = {0};
    if (readlink("/proc/self/exe", exec_file_abspath, PATH_MAX - 1) < 0) {
        LOGERR("[run_as_user] failed to get the abspath of execfile: (%d) %s", errno, errstring(errno));
        exit(errno);
    }

    if (argv && execv(exec_file_abspath, argv) < 0) {
        LOGERR("[run_as_user] failed to call execv(%s, args): (%d) %s", exec_file_abspath, errno, errstring(errno));
        exit(errno);
    }
}
