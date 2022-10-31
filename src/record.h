#ifndef RECORD_H
#define RECORD_H 1

enum control_command {
	CMD_NONE,
	CMD_CTRL_A,
	CMD_PAUSE,
};

typedef enum {
	ASCIINEMA_V1 = 1,
	ASCIINEMA_V2 = 2,
	TERMREC_V1   = 3
} fileformat_t;

struct outargs {
	int start_paused;
	int controlfd;
	int masterfd;
	int rows;
	int cols;
	fileformat_t format_version;
	int use_raw;

	const char *cmd;
	const char *env;
	const char *title;
	const char *outfn;
};

#endif
