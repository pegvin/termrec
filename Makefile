CC:=clang
CFLAGS:=-Wall -std=c99
LFLAGS:=

bin:=termrec
obj:=obj

OBJS:=$(patsubst %.c, %.o, $(wildcard src/*.c))

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
