#ifndef MAIN_H
#define MAIN_H 1

#ifdef TARGET_OSX
    #define _DARWIN_C_SOURCE 1
#endif

#ifdef TARGET_BSD
	#define _BSD_SOURCE 1
	#define _XOPEN_SOURCE 600
#endif

#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

#ifdef TARGET_BSD
    #include <time.h>
#else
	#include <sys/time.h>
#endif

void die(const char *s); // Used In Other Files Too

#endif
