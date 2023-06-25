CC:=clang
CFLAGS:=-std=c99 -Wall -O0 -g
LFLAGS:=

UNAME_S:=$(shell uname -s)

ifeq ($(findstring BSD,$(UNAME_S)),BSD)
	CFLAGS+=-DTARGET_BSD=1
	LFLAGS+=-lexecinfo
endif
ifeq ($(UNAME_S),Linux)
	CFLAGS+=-DTARGET_LINUX=1
endif
ifeq ($(UNAME_S),Darwin)
	CFLAGS+=-DTARGET_OSX=1
endif

bin:=termrec
obj:=obj

SRCS:=src/main.c src/terminal.c src/xwrap.c src/signals.c src/writer.c src/recorder.c src/play.c
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
