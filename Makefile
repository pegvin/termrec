CC:=clang
CFLAGS:=-std=c99 -Wall -O0 -g
LFLAGS:=

UNAME_S:=$(shell uname -s)
ifeq ($(UNAME_S),FreeBSD)
	LFLAGS+=-lexecinfo
endif

bin:=termrec
obj:=obj

SRCS:=src/main.c src/terminal.c src/xwrap.c src/signals.c src/writer.c
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
