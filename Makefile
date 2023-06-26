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
OBJS:=$(subst .c,.c.o,$(SRCS))

all: $(bin)

%.c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(bin): $(OBJS)
	@bear --append -- $(CC) $(CFLAGS) $(LFLAGS) $(OBJS) -o $@
	$(CC) $(CFLAGS) $(LFLAGS) $(OBJS) -o $@

# Install https://github.com/rizsotto/Bear to generate "compile_commands.json"
# Only neccessary if you want to use it with clangd or something.
bear:
	@for source in $(SRCS); do\
		bear --append -- $(CC) $(CFLAGS) -c $$source -o $$source.o ;\
	done

.PHONY: run
.PHONY: clean

run: $(all)
	./$(bin)

clean:
	$(RM) $(bin) $(OBJS)
