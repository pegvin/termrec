#include "writer.h"
#include "main.h"
#include <string.h>

FILE* outfile = NULL;
struct outargs OA;

FILE* WriterInit(const char* fileName) {
	memset(&OA, 0, sizeof(struct outargs));
	outfile = fopen(fileName, "wb");
	if (outfile == NULL) {
		die("fopen");
	}
	setbuf(outfile, NULL); // Set The Stream To Be Un-Buffered
	return outfile;
}

/*
	Function: WriteHeader()
	Description: Inits The Data Writer Which Is A Wrapper To Write Files in Multiple Formats
	Remarks: Returns Zero On Success & Negative One On Errors
*/
int WriteHeader(struct outargs* oa) {
	if (oa == NULL || outfile == NULL) return -1;

	memcpy(&OA, oa, sizeof(struct outargs));
	if (oa->format_version == ASCIINEMA_V1 || oa->format_version == ASCIINEMA_V2) {
		/* Write asciicast header and append events. Format defined at
		 * v1 https://github.com/asciinema/asciinema/blob/master/doc/asciicast-v1.md
		 * v2 https://github.com/asciinema/asciinema/blob/master/doc/asciicast-v2.md
		 *
		 * With v1, we insert an empty first record to avoid the hassle of dealing with
		 * ES (still) not supporting trailing commas.
		 */

		struct timeval tv;
		gettimeofday(&tv, NULL);

		#define _CONDITION_TAB oa->format_version == ASCIINEMA_V1 ? '\t' : ' '
		#define _CONDITION_NEWL oa->format_version == ASCIINEMA_V1 ? '\n' : ' '

		WriteStdout_fprintf("{                                                                     "); // Have Room For Putting Duration
		WriteStdout_fprintf("%c\"version\": %d,%c", _CONDITION_TAB, oa->format_version,              _CONDITION_NEWL);
		WriteStdout_fprintf("%c\"timestamp\": %ld,%c", _CONDITION_TAB, tv.tv_sec,                    _CONDITION_NEWL);
		WriteStdout_fprintf("%c\"width\": %d,%c",   _CONDITION_TAB, oa->cols,                        _CONDITION_NEWL);
		WriteStdout_fprintf("%c\"height\": %d,%c",  _CONDITION_TAB, oa->rows,                        _CONDITION_NEWL);
		WriteStdout_fprintf("%c\"command\": %s,%c", _CONDITION_TAB, oa->cmd ? oa->cmd : "\"\"",      _CONDITION_NEWL);
		WriteStdout_fprintf("%c\"title\": %s,%c",   _CONDITION_TAB, oa->title ? oa->title : "\"\"",  _CONDITION_NEWL);
		WriteStdout_fprintf("%c\"env\": %s%c%c",    _CONDITION_TAB, oa->env, oa->format_version == ASCIINEMA_V1 ? ',' : ' ', _CONDITION_NEWL);

		#undef _CONDITION_TAB
		#undef _CONDITION_NEWL

		if (oa->format_version == ASCIINEMA_V1)
			WriteStdout_fprintf("\t\"stdout\": [\n\t\t[ 0, \"\" ],\n"); // v1 header finished, console data is appended in structure
		else if (oa->format_version == ASCIINEMA_V2)
			WriteStdout_fprintf("}\n[ 0, \"o\", \"\" ]\n");
	}

	return 0;
}

int WriteDuration(float duration) {
	if (outfile == NULL) return -1;

	if (OA.format_version == ASCIINEMA_V1 || OA.format_version == ASCIINEMA_V2) {
		// seeks to header, overwriting spaces with duration
		size_t currPos = ftell(outfile);
		fseek(outfile, 2L, SEEK_SET);

		#define _CONDITION_NEWL OA.format_version == ASCIINEMA_V1 ? '\n' : ' '
		fprintf(outfile, "%c\t\"duration\": %.9g,%c", _CONDITION_NEWL, duration, _CONDITION_NEWL);
		#undef _CONDITION_NEWL

		fflush(outfile);
		fseek(outfile, currPos, SEEK_SET);
	}

	return 0;
}

void WriterClose() {
	if (OA.format_version == ASCIINEMA_V1) {
		// Delete The Comma From Last Entry in "stdout" array
		size_t currPos = ftell(outfile);
		fseek(outfile, currPos - 2, SEEK_SET);
		fputc(' ', outfile);
		fseek(outfile, currPos, SEEK_SET);

		WriteStdout_fprintf("\t]\n}\n"); // closes stdout segment
	}

	if (fclose(outfile) == -1) {
		die("fclose");
	}
	outfile = NULL;
}

void WriteStdoutStart(float duration) {
	if (outfile) {
		if (OA.format_version == ASCIINEMA_V1) {
			WriteStdout_fprintf("\t\t[ %0.4f, \"", duration);
		} else if (OA.format_version == ASCIINEMA_V2) {
			WriteStdout_fprintf("[ %0.4f, \"o\", \"", duration);
		}
	}
}

void WriteStdout_fprintf(const char* fmt, ...) {
	if (outfile) {
	    va_list args;
	    va_start(args, fmt);
	    vfprintf(outfile, fmt, args);
		va_end(args);
	}
}

void WriteStdout_fputs(const char* str) {
	if (outfile) {
		fputs(str, outfile);
	}
}

void WriteStdout_fputc(char character) {
	if (outfile) {
		fputc(character, outfile);
	}
}

void WriteStdoutEnd() {
	if (outfile) {
		if (OA.format_version == ASCIINEMA_V1) {
			WriteStdout_fputs("\" ],\n");
		} else if (OA.format_version == ASCIINEMA_V2) {
			WriteStdout_fputs("\" ]\n");
		}
	}
}
