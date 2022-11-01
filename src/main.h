#ifndef MAIN_H
#define MAIN_H 1

#define _XOPEN_SOURCE 600
#define _POSIX_C_SOURCE 1

#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1
#endif

#ifdef __FreeBSD__
#define _BSD_SOURCE 1
#define __BSD_VISIBLE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h> // For "assert()"
#include <ctype.h>
#include <inttypes.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <signal.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>

// For PATH_MAX
#ifdef __linux__
#include <linux/limits.h>
#endif
#include <limits.h>

#include "record.h"

void die(const char *s); // Used In Other Files Too

#endif
