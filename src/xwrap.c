#include "xwrap.h"
#include "main.h"

#define _XOPEN_SOURCE 500

void xclose(int fd) {
	if (close(fd) == -1) die("close");
}

int xdup2(int oldfd, int newfd) {
	int fd;
	if ((fd = dup2(oldfd, newfd)) == -1) die("dup2");
	return fd;
}

void xfclose(FILE *f) {
	if (fclose(f) == EOF) die("fclose");
}

FILE* xfopen(const char *f, const char *m) {
	FILE *r;
	r = fopen(f, m);
	if (r == NULL) die("fopen");
	return r;
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

size_t xwrite(int fd, void *buf, size_t size) {
	ssize_t s;
	if ((s = write(fd, buf, size)) == -1) die("write");
	return s;
}
