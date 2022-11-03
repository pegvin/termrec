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
} trMode_t;

typedef enum {
	ASCIINEMA_V1 = 1,
	ASCIINEMA_V2 = 2
} fileformat_t;

struct outargs {
	int start_paused;
	int controlfd;
	int masterfd;
	int rows;
	int cols;
	fileformat_t format_version;
	trMode_t mode;

	const char* cmd;
	const char* env;
	const char* title;
	const char* fileName;
};

#endif
