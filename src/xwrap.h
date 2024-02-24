#ifndef XWRAP_H
#define XWRAP_H 1

#define _XOPEN_SOURCE 600 // For Structs Like "sigaction"
#define _BSD_SOURCE 1

#define _POSIX_C_SOURCE 199309L
// Maybe Needed on Mac for `SIGWINCH`
#define _DARWIN_C_SOURCE 1
#include <signal.h>
#include <termios.h>

int  xdup2(int oldfd, int newfd);
void xsigaction(int signum, const struct sigaction* act, struct sigaction* oldact);
void xtcgetattr(int fd, struct termios* tio);
void xtcsetattr(int fd, int opt, const struct termios* tio);

#endif
