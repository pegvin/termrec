#ifndef RECORD_H
#define RECORD_H 1

enum control_command {
	CMD_NONE,
	CMD_CTRL_A,
	CMD_PAUSE,
};

struct outargs {
	int start_paused;
	int controlfd;
	int masterfd;
	int rows;
	int cols;
	int format_version;
	int use_raw;

	const char *cmd;
	const char *env;
	const char *title;
	const char *outfn;
};

#endif
