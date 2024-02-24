#ifndef RECORD_H
#define RECORD_H 1

#include <stdint.h>

enum control_command {
	CMD_NONE,
	CMD_CTRL_A,
	CMD_PAUSE,
};

typedef enum {
	TERMREC_PLAY = 1,
	TERMREC_RECORD
} AppMode;

struct Recording {
	uint32_t    width;  // cols
	uint32_t    height; // rows
	uint64_t    timestamp;
	const char* env;
	const char* filePath;
};

struct outargs {
	int start_paused;
	int controlfd;
	int masterfd;
	AppMode mode;
	struct Recording rec;
};

#endif
