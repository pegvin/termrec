#include "writer.h"
#include "xwrap.h"

FILE* outfile = NULL;
fileformat_t FormatVersion = -1;

FILE* WriterInit(const char* fileName) {
	outfile = xfopen(fileName, "wb");
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

	FormatVersion = oa->format_version;
	if (oa->format_version == ASCIINEMA_V1 || oa->format_version == ASCIINEMA_V2) {
		/* Write asciicast header and append events. Format defined at
		 * v1 https://github.com/asciinema/asciinema/blob/master/doc/asciicast-v1.md
		 * v2 https://github.com/asciinema/asciinema/blob/master/doc/asciicast-v2.md
		 *
		 * With v1, we insert an empty first record to avoid the hassle of dealing with
		 * ES (still) not supporting trailing commas.
		 */

		fprintf(outfile,
			"{                              " // have room to write duration later
			"\"version\": %d, "
			"\"width\": %d, "
			"\"height\": %d, "
			"\"command\": \"%s\", "
			"\"title\": \"%s\", "
			"\"env\": %s",
			oa->format_version,
			oa->cols, oa->rows,
			oa->cmd ? oa->cmd : "",
			oa->title ? oa->title : "",
			oa->env
		);

		if (oa->format_version == ASCIINEMA_V1)
			fprintf(outfile, ",\"stdout\":[[0,\"\"]\n"); // v1 header finished, console data is appended in structure
		else
			fprintf(outfile, "}\n"); // v2 header finished here, data will be appended in separate lines
	}

	return 0;
}

int WriteDuration(float duration) {
	if (outfile == NULL) return -1;

	// seeks to header, overwriting spaces with duration
	fseek(outfile, 1L, SEEK_SET);
	fprintf(outfile, "\"duration\": %.9g, ", duration);
	fflush(outfile);

	return 0;
}

void WriterClose() {
	if (FormatVersion == ASCIINEMA_V1) WriteStdout_fprintf("]}\n"); // closes stdout segment
	xfclose(outfile);
	outfile = NULL;
}

void WriteStdoutStart(float duration) {
	if (outfile) {
		if (FormatVersion == ASCIINEMA_V1) {
			fprintf(outfile, ",[%0.4f,\"", duration);
		} else if (FormatVersion == ASCIINEMA_V2) {
			fprintf(outfile, "[%0.4f,\"o\",\"", duration);
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
	if (outfile && (FormatVersion == ASCIINEMA_V1 || FormatVersion == ASCIINEMA_V2)) {
		fputs("\"]\n", outfile);
	}
}
