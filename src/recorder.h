#ifndef RECORDER_H
#define RECORDER_H 1

#include <sys/ioctl.h>
#include "record.h"

void StartOutputProcess(struct outargs* oa);
void StartChildShell(const char* shell, const char* exec_cmd, struct winsize* win, int masterfd);
void ProcessInputs(int masterfd, int controlfd);
char* SerializeEnv(void);
void RecordSession(struct outargs oa);

#endif
