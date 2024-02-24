#ifndef TERMINAL_H
#define TERMINAL_H 1

void TermEnableRawMode();
void TermDisableRawMode();

int TermGetWinSize(int *rows, int *cols);
int TermGetCursorPos(int *rows, int *cols);

#endif
