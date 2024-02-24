#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "recorder.h"
#include "play.h"

// Also Used in signals.c & recorder.c
struct winsize owin, rwin, win;
FILE *debug_out;
int masterfd;
pid_t child;

void PrintUsage(const char* name) {
	printf("Usage: %s [command] [session-path] [options]\n\n", name);
	printf("[command]\n  - play: play a session\n  - rec: record a session\n  - help: shows help message\n\n");
	printf("[options]\n  -f, --format: file-format of the session being played (default asciinema_v1)\n                valid formats are: asciinema_v1, asciinema_v2\n");
}

int main(int argc, char** argv) {
	if (argc <= 1) {
		PrintUsage(argc < 1 ? "termrec" : argv[0]);
		exit(EXIT_FAILURE);
	}

	struct outargs oa;
	memset(&oa, 0, sizeof oa);

	unsigned char i = 1;

	if (strcmp(argv[i], "rec") == 0) {
		if (i == argc - 1) { printf("No session name specified!\n"); exit(EXIT_FAILURE); }
		oa.mode = TERMREC_RECORD;
		oa.fileName = argv[i + 1];
	} else if (strcmp(argv[i], "play") == 0) {
		if (i == argc - 1) { printf("No session name specified!\n"); exit(EXIT_FAILURE); }
		oa.mode = TERMREC_PLAY;
		oa.fileName = argv[i + 1];
	} else if (strcmp(argv[i], "help") == 0) {
		PrintUsage(argv[0]);
		return 0;
	} else {
		printf("\"%s\" is a invalid command!\n", argv[i]);
		exit(EXIT_FAILURE);
	}

	for (i = 2; i < argc; i++) {
		if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) {
			if (i == argc - 1) { printf("No file-format name specified!\n"); exit(EXIT_FAILURE); }
			const char* format = argv[i + 1];
			if (strcmp(format, "asciinema_v1") == 0) oa.format_version = ASCIINEMA_V1;
			else if (strcmp(format, "asciinema_v2") == 0) oa.format_version = ASCIINEMA_V2;
			else { printf("Invalid file-format specified!\n"); exit(EXIT_FAILURE); }
		}
	}

	if (oa.mode == TERMREC_RECORD) {
		RecordSession(oa);
	} else if (oa.mode == TERMREC_PLAY) {
		PlaySession(oa);
	}

	return 0;
}

// Simple Wrapper For perror & exit which resets the screen too...
void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4); // erase entire screen
	write(STDOUT_FILENO, "\x1b[H", 3);  // move cursor to home position (0, 0)

	perror(s);
	exit(EXIT_FAILURE);
}
