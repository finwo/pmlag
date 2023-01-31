# Initial config
LIBS=
SRC=$(wildcard src/*.c)

override CFLAGS?=-Wall

INCLUDES:=
INCLUDES+=-I src

include lib/.dep/config.mk

override CFLAGS+=$(INCLUDES)

# Which objects to generate before merging everything together
OBJ:=$(SRC:.c=.o)

default: pmlag

%.o: %.c $(LIBS)
	$(CC) $(CFLAGS) $(@:.o=.c) -c -o $@

pmlag: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

.PHONY: clean
clean:
	rm -f $(OBJ)
