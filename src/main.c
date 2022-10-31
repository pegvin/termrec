#include "main.h"
#include "terminal.h"
#include "record.h"
#include "xwrap.h"
#include "utf8.h"

// Also Used in signals.c
struct winsize owin, rwin, win;
FILE *debug_out;
int masterfd;
pid_t child;

extern char** environ;
static int paused, start_paused;
static struct timeval prevtv, nowtv;
static double dur;  // Duration
static FILE* evout; // Output File
static int master;  // Master/Parent FD

// Forward Decls...
void StartOutputProcess(struct outargs *oa);
void StartChildShell(const char *shell, const char *exec_cmd, struct winsize *win, int masterfd);
void ProcessInputs(int masterfd, int controlfd);
char* SerializeEnv(void);

int main(int argc, char** argv) {
	SetupSignalHandlers(); // In signals.c

	int rows = 0; // Width
	int cols = 0; // Height

	int controlfd[2];
	extern char *optarg;
	extern int optind;
	struct outargs oa;
	char *exec_cmd;
	char cwd[PATH_MAX];

	memset(&oa, 0, sizeof oa);
	oa.env = SerializeEnv();
	exec_cmd = NULL;

	if (!oa.format_version) {
		oa.format_version = ASCIINEMA_V2;
	}

	oa.outfn = "events.cast";

	if (pipe(controlfd) != 0) die("pipe");
	oa.controlfd = controlfd[0];

	TermGetWinSize(&rows, &cols);
	oa.rows = rows;
	oa.cols = cols;

	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("CWD: %s\n", cwd);
	} else {
		perror("getcwd() error");
		return 1;
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
			xclose(controlfd[1]);
			StartOutputProcess(&oa);
		} else {
			char *shell = getenv("SHELL");
			if (shell == NULL) {
				shell = "/bin/sh";
			}

			/* Shell process doesn't need these */
			xclose(controlfd[0]);
			xclose(controlfd[1]);

			/* Run the shell in the child */
			StartChildShell(shell, exec_cmd, &win, masterfd);
		}
	}

	xclose(controlfd[0]);
	ProcessInputs(masterfd, controlfd[1]); // "Main Loop"

	signal(SIGWINCH, NULL);
	if (ioctl(STDIN_FILENO, TIOCSWINSZ, &rwin) == -1) die("ioctl(TIOCSWINSZ)");

	TermDisableRawMode();

	return 0;
}

// Simple Wrapper For perror & exit which resets the screen too...
void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4); // erase entire screen
	write(STDOUT_FILENO, "\x1b[H", 3);  // move cursor to home position (0, 0)

	perror(s);
	exit(EXIT_FAILURE);
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
		xwrite(master, &c_a, 1);
		break;

	case CMD_PAUSE:
		paused = !paused;
		if (!paused) {
			/* Redraw screen */
			xwrite(master, &c_l, 1);
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
	assert(format_version >= ASCIINEMA_V1 && format_version <= TERMREC_V1);
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

	if (format_version == ASCIINEMA_V2) {
		fprintf(evout, "[%0.4f,\"o\",\"", dur / 1000);
	} else if (format_version == ASCIINEMA_V1) {
		fprintf(evout, ",[%0.4f,\"", delta / 1000);
	}

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
						fprintf(evout, "\\u%04" PRIx32 "\\u%04" PRIx32, h, l);
					} else {
						fprintf(evout, "\\u%04" PRIx32, cp);
					}
				} else {
					fputs("\\ud83d\\udca9", evout);
				}
			} else {
				switch (buf[j]) {
				case '"':
				case '\\':
					fputc('\\', evout); // output backslash for escaping
					fputc(buf[j], evout); // print the character itself
					break;
				default:
					fputc(buf[j], evout);
					break;
				}
			}
		}
	}

	if (format_version == ASCIINEMA_V1 || format_version == ASCIINEMA_V2) fputs("\"]\n", evout);
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

	assert(oa->format_version >= ASCIINEMA_V1 && oa->format_version <= TERMREC_V1);
	// assert(oa->format_version == 1 || oa->format_version == 2);

	start_paused = paused = oa->start_paused;

	evout = xfopen(oa->outfn, "wb");

	/* Write asciicast header and append events. Format defined at
	 * v1 https://github.com/asciinema/asciinema/blob/master/doc/asciicast-v1.md
	 * v2 https://github.com/asciinema/asciinema/blob/master/doc/asciicast-v2.md
	 *
	 * With v1, we insert an empty first record to avoid the hassle of dealing with
	 * ES (still) not supporting trailing commas.
	 */
	fprintf(evout,
	    "{                        " // have room to write duration later
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
	if (oa->format_version == ASCIINEMA_V2) {
		// v2 header finished here, data will be appended in separate lines
		fprintf(evout, "}\n");
	} else if (oa->format_version == ASCIINEMA_V1) {
		// v1 header finished, console data is appended in structure
		fprintf(evout, ",\"stdout\":[[0,\"\"]\n");
	}

	setbuf(evout, NULL);
	setbuf(stdout, NULL);

	xclose(STDIN_FILENO);

	/* Clear screen */
	printf("\x1b[2J");

	/* Move cursor to top-left */
	printf("\x1b[H");

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

				xwrite(STDOUT_FILENO, obuf, nread);

				if (!paused) {
					handle_input(obuf, nread, oa->format_version);
				}
			}
		}
	}

end:
	if (oa->format_version == ASCIINEMA_V1) {
		// closes stdout segment
		fprintf(evout, "]}\n");
	}
	// seeks to header, overwriting spaces with duration
	fseek(evout, 1L, SEEK_SET);
	fprintf(evout, "\"duration\": %.9g, ", dur / 1000);

	fflush(evout);

	xfclose(evout);
	xclose(oa->masterfd);

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
					if (cmdstart > ibuf) xwrite(masterfd, ibuf, cmdstart - ibuf);

					cmdstart++;
					nread -= (cmdstart - ibuf);
					p = cmdstart;
					input_state = STATE_COMMAND;
				}
				xwrite(masterfd, ibuf, nread);
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
						xwrite(controlfd, &cmd, sizeof cmd);
					}

					if (nread > 1) {
						xwrite(masterfd, p + 1, nread - 1);
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

	xclose(masterfd);

	xdup2(slavefd, 0);
	xdup2(slavefd, 1);
	xdup2(slavefd, 2);

	xclose(slavefd);

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

