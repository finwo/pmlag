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

BIN=pmlag

.PHONY: default
default: $(BIN) $(BIN).1 README.md

%.o: %.c $(LIBS)
	$(CC) $(CFLAGS) $(@:.o=.c) -c -o $@

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

# Build manpage
$(BIN).1: manpage.1.md
	env NAME=$(BIN) envsubst < manpage.1.md | pandoc --standalone --from markdown --to man -o $(BIN).1
	env NAME=$(BIN) envsubst < manpage.1.md | pandoc --standalone --from markdown --to html -o $(BIN).html

README.md: manpage.1.md
	env NAME=$(BIN) envsubst < manpage.1.md > README.md

.PHONY: clean
clean:
	rm -f $(OBJ)
