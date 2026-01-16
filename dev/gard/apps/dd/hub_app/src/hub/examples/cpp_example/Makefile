CPP=g++
CPPFLAGS=-Wall -Werror
CPPFLAGS+=-I/opt/hub/include

LDFLAGS=-L/opt/hub/lib -lhub -lusb-1.0 -lcjson
LDFLAGS+=-Wl,-rpath=/opt/hub/lib

SRCS:=$(wildcard *.cpp)
OBJS:=$(patsubst %.cpp,%.o,$(SRCS))

ELF:=a.out

.PHONY: all clean

all: $(ELF)

%.o: %.c
	$(CPP) $(CFLAGS) -c $^ -o $@

$(ELF): $(OBJS)
	$(CPP) $^ $(LDFLAGS) -o $@

clean:
	rm -rf $(ELF) $(OBJS)