CC:=clang
CFLAGS:=-std=c99 -Wall -O0 -g
LFLAGS:=

bin:=termrec
obj:=obj

SRCS:=src/main.c src/terminal.c src/xwrap.c src/signals.c
OBJS:=$(SRCS:.c=.o)

all: $(bin)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(bin): $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $(OBJS) -o $@

.PHONY: run
.PHONY: clean

run: $(all)
	./$(bin)

clean:
	$(RM) $(bin) $(OBJS)
