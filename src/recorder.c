#include "recorder.h"

void SetupSignalHandlers(void); // In signals.c

// Defined In main.c
extern struct winsize owin, rwin, win;
extern FILE *debug_out;
extern int masterfd;
extern pid_t child;

// https://man7.org/linux/man-pages/man7/environ.7.html
extern char** environ;

static int paused, start_paused;
static struct timeval prevtv, nowtv;
static double dur;  // Duration
static int master;  // Master/Parent FD

void RecordSession(struct outargs oa) {
	SetupSignalHandlers();

	int rows = 0; // Width
	int cols = 0; // Height

	int controlfd[2];
	extern char *optarg;
	extern int optind;
	char *exec_cmd;
	char cwd[PATH_MAX];

	oa.env = SerializeEnv();
	exec_cmd = NULL;

	if (!oa.format_version) oa.format_version = ASCIINEMA_V1;
	if (!oa.fileName) oa.fileName = "events.cast";

	if (pipe(controlfd) != 0) die("pipe");
	oa.controlfd = controlfd[0];

	TermGetWinSize(&rows, &cols);
	oa.rows = rows;
	oa.cols = cols;

	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("CWD: %s\n", cwd);
	} else {
		perror("getcwd() error");
		return;
	}

	oa.masterfd = masterfd = posix_openpt(O_RDWR | O_NOCTTY);
	if (masterfd == -1) die("posix_openpt");

	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &win) == -1) die("ioctl(TIOCGWINSZ)");

	owin = rwin = win;
	if (!oa.rows || oa.rows > win.ws_row) oa.rows = win.ws_row;
	if (!oa.cols || oa.cols > win.ws_col) oa.cols = win.ws_col;

	win.ws_row = oa.rows;
	win.ws_col = oa.cols;

	TermEnableRawMode();

	child = fork();
	if (child < 0) die("fork");
	else if (child == 0) {
		pid_t subchild = fork();
		if (subchild < 0) {
			perror("fork");
			if (kill(0, SIGTERM) == -1) perror("kill");
			exit(EXIT_FAILURE);
		}

		signal(SIGWINCH, SIG_IGN);

		if (subchild) {
			/* Handle output to file in parent */
			if (close(controlfd[1]) == -1) {
				die("close");
			}
			StartOutputProcess(&oa);
		} else {
			char *shell = getenv("SHELL");
			if (shell == NULL) {
				shell = "/bin/sh";
			}

			/* Shell process doesn't need these */
			if (close(controlfd[0]) == -1) {
				die("close");
			}
			if (close(controlfd[1]) == -1) {
				die("close");
			}

			/* Run the shell in the child */
			StartChildShell(shell, exec_cmd, &win, masterfd);
		}
	}

	if (close(controlfd[0]) == -1) {
		die("close");
	}
	ProcessInputs(masterfd, controlfd[1]); // "Main Loop"

	signal(SIGWINCH, NULL);
	if (ioctl(STDIN_FILENO, TIOCSWINSZ, &rwin) == -1) die("ioctl(TIOCSWINSZ)");

	TermDisableRawMode();
}


// small function which escapes '"'
static char* escape(const char *from) {
	unsigned i, o;
	char *p;

	assert(from);

	p = malloc(strlen(from) * 2 + 1);
	if (p == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	for (i = 0, o = 0; i < strlen(from); i++) {
		switch (from[i]) {
		case '\\':
		case '"':
			p[o++] = '\\';
			p[o++] = from[i];
			break;
		default:
			p[o++] = from[i];
		}
	}

	p[o] = '\0';

	return p;
}

/*
	Function: SerializeEnv
	Description: Converts Environment Variables To A JSON String
				 And Returns The String.
				 The Returned String Isn't Doesn't Need To Be Free-d
*/
char* SerializeEnv(void) {
	static const char *interesting[] = {
		"TERM",
		"SHELL",
		"PS1",
		"PS2",
	};
	size_t o = 0, s = 1024;
	char *p;

	p = malloc(s);
	if (p == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	p[o++] = '{';

	for (unsigned i = 0; environ[i] != NULL; i++) {
		char *e, *k, *v;
		int useful, rv;
		size_t l;

		e = escape(environ[i]);

		k = e;
		v = strchr(e, '=');
		if (v == NULL) {
			fprintf(stderr, "Your environment is whack.\n");
			exit(EXIT_FAILURE);
		}

		*v++ = '\0';

		useful = 0;
		for (unsigned j = 0; j < sizeof interesting / sizeof interesting[0]; j++) {
			if (strcmp(k, interesting[j]) == 0) {
				useful = 1;
				break;
			}
		}

		if (!useful) {
			goto not_useful;
		}

		l = strlen(k) + strlen(v);
		/* 7 is for two sets of quotes, a colon, a comma, and a 0 byte */
		if (o + l + 7 >= s) {
			size_t ns;
			char *r;

			ns = s + l + 1025;
			r = realloc(p, ns);
			if (r == NULL) {
				perror("realloc");
				exit(EXIT_FAILURE);
			}

			s = ns;
			p = r;
		}

		rv = snprintf(&p[o], s - o, "\"%s\":\"%s\",", k, v);
		assert(rv > 0);
		assert((unsigned)rv < s - o);
		o += rv;

not_useful:	free(e);
	}

	if (p[o - 1] == ',') {
		p[o - 1] = '}';
	} else {
		p[o] = '}';
	}

	return p;
}

// Simple Function Which Handles termrec Commands Like Ctrl + P
static void handle_command(enum control_command cmd) {
	static unsigned char c_a = 0x01;
	static unsigned char c_l = 0x0c;

	switch (cmd) {
	case CMD_CTRL_A:
		/* Can't just write this to STDOUT_FILENO; this has to be
		 * written to the master end of the tty, otherwise it's 
		 * ignored.
		 */
		if (write(master, &c_a, 1) == -1) {
			die("write");
		}
		break;

	case CMD_PAUSE:
		paused = !paused;
		if (!paused) {
			/* Redraw screen */
			if (write(master, &c_l, 1) == -1) {
				die("write");
			}
			gettimeofday(&prevtv, NULL);
			nowtv = prevtv;
		}
		break;

	default:
		abort();
	}
}

// This Function Is Responsible For Writing The Data To The File
static inline void handle_input(unsigned char *buf, size_t buflen, fileformat_t format_version) {
	assert(format_version >= ASCIINEMA_V1 && format_version <= ASCIINEMA_V2);
	static int first = 1;
	double delta;

	if (first) {
		gettimeofday(&prevtv, NULL);
		nowtv = prevtv;
		first = 0;
	} else {
		gettimeofday(&nowtv, NULL);
	}

	double pms, nms;
	pms = (double)(prevtv.tv_sec * 1000) + ((double)prevtv.tv_usec / 1000.);
	nms = (double)(nowtv.tv_sec * 1000) + ((double)nowtv.tv_usec / 1000.);

	delta = nms - pms;
	prevtv = nowtv;

	dur += delta;

	WriteStdoutStart((format_version == ASCIINEMA_V1 ? delta : dur) / 1000);

	uint32_t state, cp;
	state = 0;
	for (size_t j = 0; j < buflen; j++) {
		if (!u8_decode(&state, &cp, buf[j])) {
			if ((cp < 128 && !isprint(cp)) ||
				cp > 128) {
				if (state == UTF8_ACCEPT) {
					if (cp > 0xffff) {
						uint32_t h, l;
						h = ((cp - 0x10000) >> 10) + 0xd800;
						l = ((cp - 0x10000) & 0x3ff) + 0xdc00;
						WriteStdout_fprintf("\\u%04" PRIx32 "\\u%04" PRIx32, h, l);
					} else {
						WriteStdout_fprintf("\\u%04" PRIx32, cp);
					}
				} else {
					WriteStdout_fputs("\\ud83d\\udca9");
				}
			} else {
				switch (buf[j]) {
				case '"':
				case '\\':
					WriteStdout_fputc('\\'); // output backslash for escaping
					WriteStdout_fputc(buf[j]); // print the character itself
					break;
				default:
					WriteStdout_fputc(buf[j]);
					break;
				}
			}
		}
	}

	WriteStdoutEnd();
}

/*
	Function: StartOutputProcess()
	Description: The Function Starts Capturing The Data Given To Master By The Child And Writes It To A File
*/
void StartOutputProcess(struct outargs *oa) {
	unsigned char obuf[BUFSIZ];
	struct pollfd pollfds[2];
	int status;

	status = EXIT_SUCCESS;
	master = oa->masterfd;

	assert(oa->format_version >= ASCIINEMA_V1 && oa->format_version <= ASCIINEMA_V2);

	start_paused = paused = oa->start_paused;

	setbuf(stdout, NULL); // Set The Stream To Be Un-Buffered
	WriterInit(oa->fileName);
	WriteHeader(oa);

	if (close(STDIN_FILENO) == -1) {
		die("close");
	}

	printf("\x1b[2J"); // Clear screen
	printf("\x1b[H");  // Move cursor to top-left

	int f = fcntl(oa->masterfd, F_GETFL);
	fcntl(oa->masterfd, F_SETFL, f | O_NONBLOCK);

	f = fcntl(oa->controlfd, F_GETFL);
	fcntl(oa->controlfd, F_SETFL, f | O_NONBLOCK);

	/* Control descriptor is highest priority */
	pollfds[0].fd = oa->controlfd;
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;

	pollfds[1].fd = oa->masterfd;
	pollfds[1].events = POLLIN;
	pollfds[1].revents = 0;

	while (1) {
		int nready;

		nready = poll(pollfds, 2, -1);
		if (nready == -1 && errno == EINTR) {
			continue;
		} else if (nready == -1) {
			perror("poll");
			status = EXIT_FAILURE;
			goto end;
		}

		for (int i = 0; i < 2; i++) {
			if ((pollfds[i].revents & (POLLHUP | POLLERR | POLLNVAL))) {
				status = EXIT_FAILURE;
				goto end;
			}

			if (!(pollfds[i].revents & POLLIN)) {
				continue;
			}

			if (pollfds[i].fd == oa->controlfd) {
				enum control_command cmd;
				ssize_t nread;

				nread = read(oa->controlfd, &cmd, sizeof cmd);
				if (nread == -1 || nread != sizeof cmd) {
					perror("read");
					status = EXIT_FAILURE;
					goto end;
				}

				handle_command(cmd);
			} else if (pollfds[i].fd == oa->masterfd) {
				ssize_t nread;

				nread = read(oa->masterfd, obuf, BUFSIZ);
				if (nread <= 0) {
					status = EXIT_FAILURE;
					goto end;
				}

				if (write(STDOUT_FILENO, obuf, nread) == -1) {
					die("write");
				}

				if (!paused) {
					handle_input(obuf, nread, oa->format_version);
				}
			}
		}
	}

end:
	WriteDuration(dur / 1000);
	WriterClose();
	if (close(oa->masterfd) == -1) {
		die("close");
	}

	printf("Session Recorded Successfully!\r\n");
	exit(status);
}

/*
	Function: ProcessInputs()
	Description: Processes The User Input, Like Ctrl + P For Pausing...
*/
void ProcessInputs(int masterfd, int controlfd) {
	unsigned char ibuf[BUFSIZ], *p;
	enum input_state { STATE_PASSTHROUGH, STATE_COMMAND } input_state;
	ssize_t nread;

	input_state = STATE_PASSTHROUGH;

	while ((nread = read(STDIN_FILENO, ibuf, BUFSIZ)) > 0) {
		p = ibuf;
		unsigned char* cmdstart = memchr(ibuf, 0x01, nread);

		switch (input_state) {
			// The The Input Is Not A Command Just Write To The Master
			case STATE_PASSTHROUGH: {
				if (cmdstart) {
					// Switching into command mode: pass through anything preceding our command
					if (cmdstart > ibuf) {
						if (write(masterfd, ibuf, cmdstart - ibuf) == -1) {
							die("write");
						}
					}

					cmdstart++;
					nread -= (cmdstart - ibuf);
					p = cmdstart;
					input_state = STATE_COMMAND;
				}
				if (write(masterfd, ibuf, nread) == -1) {
					die("write");
				}
				break;
			}
			// If The Input is a Command Check Which Command It Is And If Unknown Just Write To The Master If Known Just Execute It
			case STATE_COMMAND: {
				if (nread) {
					enum control_command cmd = CMD_NONE;

					switch (*p) {
					/* Passthrough a literal ^a */
					case 'a':
					case  0x01:
						cmd = CMD_CTRL_A;
						break;
					case 'p':
						cmd = CMD_PAUSE;
						break;
					default:
						input_state = STATE_PASSTHROUGH;
						break;
					}

					if (cmd != CMD_NONE) {
						if (write(controlfd, &cmd, sizeof cmd) == -1) {
							die("write");
						}
					}

					if (nread > 1) {
						if (write(masterfd, p + 1, nread - 1) == -1) {
							die("write");
						}
					}

					input_state = STATE_PASSTHROUGH;
				}
				break;
			}
		}
	}
}

/*
	Function: StartChildShell()
	Description: Starts A Shell In The Child P-Terminal
*/
void StartChildShell(const char *shell, const char *exec_cmd, struct winsize *win, int masterfd) {
	char *pt_name;
	int slavefd;

	slavefd = -1;

	if (setsid() == -1) {
		perror("setsid");
		goto end;
	}

	if (grantpt(masterfd) == -1) {
		perror("grantpt");
		goto end;
	}

	if (unlockpt(masterfd) == -1) {
		perror("unlockpt");
		goto end;
	}

	pt_name = ptsname(masterfd);
	if (pt_name == NULL) {
		perror("ptsname");
		goto end;
	}

	if ((slavefd = open(pt_name, O_RDWR)) < 0) {
		perror("open(ptsname)");
		goto end;
	}

	if (ioctl(slavefd, TIOCSCTTY, 0) == -1) {
		perror("ioctl(TIOCSCTTY)");
		goto end;
	}

	if (ioctl(slavefd, TIOCSWINSZ, win) == -1) {
		perror("ioctl(TIOCSWINSZ)");
		goto end;
	}

	if (close(masterfd) == -1) {
		die("close");
	}

	xdup2(slavefd, 0);
	xdup2(slavefd, 1);
	xdup2(slavefd, 2);

	if (close(slavefd) == -1) {
		die("close");
	}

	if (!exec_cmd) {
		execl(shell, strrchr(shell, '/') + 1, "-i", NULL);
	} else {
		execl(shell, strrchr(shell, '/') + 1, "-c", exec_cmd, NULL);
	}

	perror("execl");

end:
	if (kill(0, SIGTERM) == -1) {
		perror("kill");
	}
}

