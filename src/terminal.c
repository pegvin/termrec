#include <stdio.h>
#include "terminal.h"
#include "main.h"

struct termios OriginalTermIOS;

void TermEnableRawMode() {
	if (tcgetattr(STDIN_FILENO, &OriginalTermIOS) == -1) die("tcgetattr");

	struct termios raw = OriginalTermIOS;

	raw.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|INLCR|IGNCR|ICRNL|IXON);
	raw.c_oflag &= ~OPOST;
	raw.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	raw.c_cflag &= ~(CSIZE|PARENB);
	raw.c_cflag |= CS8;
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

void TermDisableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &OriginalTermIOS) == -1)
		die("tcsetattr");
}

// Uses Simple Escape Codes To Get The Cursor Positon
int TermGetCursorPos(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

	return 0;
}

// Uses Simple Escape Codes To Get The Window Size in Rows & Columns & Not Pixels
int TermGetWinSize(int *rows, int *cols) {
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
		return TermGetCursorPos(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}
