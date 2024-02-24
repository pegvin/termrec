#include "xwrap.h"
#include "main.h"

int xdup2(int oldfd, int newfd) {
	int fd;
	if ((fd = dup2(oldfd, newfd)) == -1) die("dup2");
	return fd;
}

void xsigaction(int signum, const struct sigaction* act, struct sigaction* oldact) {
	if (sigaction(signum, act, oldact) == -1) die("sigaction");
}

void xtcgetattr(int fd, struct termios *tio) {
	if (tcgetattr(fd, tio) == -1) die("tcgetattr");
}

void xtcsetattr(int fd, int opt, const struct termios *tio) {
	if (tcsetattr(fd, opt, tio) == -1) die("tcsetattr");
}

