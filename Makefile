CC:=clang
CFLAGS:=-std=c99 -Wall -O0 -g
LFLAGS:=
CLI_CFLAGS:=
CLI_LFLAGS:=

bin:=termrec
obj:=obj

SRCS:=src/main.c src/terminal.c src/xwrap.c src/signals.c src/writer.c
OBJS:=$(SRCS:.c=.o)

all: $(bin)

%.o: %.c
	$(CC) $(CFLAGS) $(CLI_CFLAGS) -c $< -o $@

$(bin): $(OBJS)
	$(CC) $(CFLAGS) $(CLI_CFLAGS) $(LFLAGS) $(CLI_LFLAGS) $(OBJS) -o $@

.PHONY: run
.PHONY: clean

run: $(all)
	./$(bin)

clean:
	$(RM) $(bin) $(OBJS)
