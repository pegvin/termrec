#ifndef WRITER_H
#define WRITER_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "record.h"

// Initializes Writer, And Returns The FILE* in which all the data is written
FILE* WriterInit(const char* fileName);

// Write The Header Like Columns & Rows, Title Etc...
int WriteHeader(struct outargs* oa);

// Write The Duration Of The Recording
int WriteDuration(float duration);

// Call To Close The Writer
void WriterClose();

/*
 * Wrappers Around fprintf, fputs, fputc to write data depending on the file format
 */

// Call Before Writing A New STDOUT Data
void WriteStdoutStart(float duration);

// Call To Write STDOUT Data
void WriteStdout_fprintf(const char* fmt, ...);
void WriteStdout_fputs(const char* str);
void WriteStdout_fputc(char character);

// Call After All The STDOUT Data is Written
void WriteStdoutEnd();

#endif
