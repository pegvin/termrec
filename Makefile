CC:=gcc
CFLAGS:=-std=c99 -Wall -MMD -MP
LFLAGS:=

BUILD      := build
BUILD_TYPE := Debug
BIN        := $(BUILD)/termrec

# Comment This out if you don't want to/don't have bear Installed
# Bear is a lovely utility to generate 'compile_commands.json'
# which then your LSP can use to provide code completions.
# https://github.com/rizsotto/Bear
BEAR       := bear --append --output $(BUILD)/compile_commands.json --

SOURCES    := src/main.c src/terminal.c src/signals.c src/writer.c src/recorder.c src/xwrap.c src/play.c
OBJECTS    := $(SOURCES:.c=.c.o)
OBJECTS    := $(patsubst %,$(BUILD)/%,$(OBJECTS))
DEPENDS    := $(OBJECTS:.o=.d)

ifeq ($(BUILD_TYPE),Debug)
	CFLAGS+=-O0 -g
else
	ifeq ($(BUILD_TYPE),Release)
		CFLAGS+=-O3
	else
$(error Unknown Build Type "$(BUILD_TYPE)")
	endif
endif

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

-include $(DEPENDS)

all: $(BIN)

$(BUILD)/%.c.o: %.c
	@echo "CC -" $<
	@mkdir -p "$$(dirname "$@")"
	@$(BEAR) $(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJECTS)
	@echo "LD -" $@
	@$(CC) $(OBJECTS) $(LFLAGS) -o $@

.PHONY: run
.PHONY: clean

run: all
	@./$(BIN)

clean:
	@$(RM) -rv $(BIN) $(BUILD)

