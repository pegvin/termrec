#include "writer.h"
#include "main.h"
#include <string.h>
#include <sys/time.h>

FILE* Writer_Init(const char* filePath) {
	FILE* f = fopen(filePath, "wb");
	if (f == NULL) {
		die("fopen");
	}
	setbuf(f, NULL); // Set The Stream To Be Un-Buffered
	return f;
}

int Writer_WriteHeader(FILE* file, const struct Recording* rec) {
	if (file == NULL || rec == NULL) return -1;

	struct timeval tv;
	gettimeofday(&tv, NULL);

	fprintf(
		file,
		"{\n"
		"\t\"version\": 1,\n"
		"\t\"timestamp\": %ld,\n"
		"\t\"width\": %d,\n"
		"\t\"height\": %d,\n"
		"\t\"command\": \"\",\n"
		"\t\"title\": \"\",\n"
		"\t\"env\": %s,\n"
		"\t\"stdout\": [\n\t\t[ 0, \"\" ],\n", // v1 header finished, console data is appended in structure
		tv.tv_sec,
		rec->width,
		rec->height,
		rec->env
	);

	return 0;
}

int Writer_WriteDuration(FILE* file, float duration) {
	if (file == NULL) return -1;

	// Go back 6 characters and replace the ',' at that
	// position to a ' ', as the item before it would be
	// the end of the JSON Array, so extraneous ',' would
	// cause syntax error
	size_t curr = ftell(file);
	fseek(file, curr - 6, SEEK_SET);
	fputc(' ', file);
	fseek(file, curr, SEEK_SET);

	fprintf(file, "\t\"duration\": %.9g\n", duration);

	return 0;
}

void Writer_Close(FILE* file) {
	fputs("}\n", file);
	if (fclose(file) == -1) {
		die("fclose");
	}
}

void Writer_OnBeforeStdoutData(FILE* file, float duration) {
	fprintf(file, "\t\t[ %0.4f, \"", duration);
}

void Writer_OnStdoutData(FILE* file, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(file, fmt, args);
	va_end(args);
}

void Writer_OnAfterStdoutData(FILE* file) {
	fputs("\" ],\n", file);
}

void Writer_OnStdoutAllEnd(FILE* file) {
	// closes stdout segment
	fprintf(file, "\t],\n");
}
