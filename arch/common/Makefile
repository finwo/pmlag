TARGET?=
SVC?=

# Initial config
LIBS=
SRC:=
SRC+=$(wildcard src/*.c)
SRC+=$(wildcard src/*/*.c)

override CFLAGS?=-Wall -s -O2

INCLUDES:=
INCLUDES+=-I src

include lib/.dep/config.mk

override CFLAGS+=$(INCLUDES)

# Which objects to generate before merging everything together
OBJ:=$(SRC:.c=.o)

BIN=pmlag

.PHONY: default
default: $(BIN) $(BIN).1

include Makefile.pkg

%.o: %.c $(LIBS)
	$(CC) $(CFLAGS) $(@:.o=.c) -D__NAME=\"$(BIN)\" -c -o $@

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

# Build manpage
$(BIN).1: manpage.1.md
	env NAME=$(BIN) envsubst < manpage.1.md | pandoc --standalone --from markdown --to man -o $(BIN).1

${BIN}-${SVC}-${TARGET}.tar.gz: $(BIN)
	package/${SVC}/build.sh
