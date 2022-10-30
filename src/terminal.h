#ifndef TERMINAL_H
#define TERMINAL_H 1

#include <stdio.h>
#include <unistd.h> // FOR FILENOs And Various Functions Like "write"
#include <termios.h>
#include <sys/ioctl.h>

void TermEnableRawMode();
void TermDisableRawMode();

int TermGetWinSize(int *rows, int *cols);
int TermGetCursorPos(int *rows, int *cols);

#endif
