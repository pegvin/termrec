#ifndef XWRAP_H
#define XWRAP_H 1

#define _XOPEN_SOURCE 600 // For Structs Like "sigaction"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

void xclose(int fd);
int xdup2(int oldfd, int newfd);
void xfclose(FILE* f);
FILE* xfopen(const char *f, const char *m);
void xsigaction(int signum, const struct sigaction* act, struct sigaction* oldact);
void xtcgetattr(int fd, struct termios *tio);
void xtcsetattr(int fd, int opt, const struct termios *tio);
size_t xwrite(int fd, void *buf, size_t size);

#endif
