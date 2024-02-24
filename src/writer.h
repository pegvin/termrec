#ifndef WRITER_H
#define WRITER_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "record.h"

/* This is the Asciinema v1 Writer, It's Format defined at
 * v1 https://github.com/asciinema/asciinema/blob/master/doc/asciicast-v1.md */

// Initializes Writer
FILE* Writer_Init(const char* filePath);

int Writer_WriteHeader(FILE* file, const struct Recording* rec);

// Call before any stdout data arrives
void Writer_OnBeforeStdoutData(FILE* file, float duration);

// Call for any stdout data that arrives
void Writer_OnStdoutData(FILE* file, const char* fmt, ...);

// Call after there is no more stdout data
void Writer_OnAfterStdoutData(FILE* file);

// Call after ALL of the stdout data has been finally written
void Writer_OnStdoutAllEnd(FILE* file);

// Call after `Writer_OnStdoutAllEnd`, Writes The Duration Of The Recording
int Writer_WriteDuration(FILE* file, float duration);

// Call after `WriteDuration`, Finalizes the Recording
void Writer_Close(FILE* file);

#endif
