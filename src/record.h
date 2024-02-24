#ifndef RECORD_H
#define RECORD_H 1

enum control_command {
	CMD_NONE,
	CMD_CTRL_A,
	CMD_PAUSE,
};

typedef enum {
	TERMREC_PLAY = 1,
	TERMREC_RECORD
} AppMode;

typedef enum {
	ASCIINEMA_V1 = 1,
	ASCIINEMA_V2 = 2
} FileFormat;

struct outargs {
	int start_paused;
	int controlfd;
	int masterfd;

	// Terminal Rows/Columns
	int rows, cols;

	AppMode mode;
	FileFormat format;

	const char* cmd;
	const char* env;
	const char* title;
	const char* fileName;
};

#endif
