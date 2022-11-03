#ifndef RECORDER_H
#define RECORDER_H 1

#include "main.h"
#include "record.h"
#include "terminal.h"
#include "record.h"
#include "xwrap.h"
#include "utf8.h"
#include "writer.h"

void StartOutputProcess(struct outargs* oa);
void StartChildShell(const char* shell, const char* exec_cmd, struct winsize* win, int masterfd);
void ProcessInputs(int masterfd, int controlfd);
char* SerializeEnv(void);
void RecordSession(struct outargs oa);

#endif