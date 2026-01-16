CC=gcc
CFLAGS=-Wall -Werror
CFLAGS+=-I/opt/hub/include

LDFLAGS=-L/opt/hub/lib -lhub -lusb-1.0 -lcjson
LDFLAGS+=-Wl,-rpath=/opt/hub/lib

SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))

ELF:=a.out

.PHONY: all clean

all: $(ELF)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(ELF): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	rm -rf $(ELF) $(OBJS)