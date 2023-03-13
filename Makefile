# Initial config
LIBS=
SRC:=
SRC+=$(wildcard src/*.c)
SRC+=$(wildcard src/*/*.c)

override CFLAGS?=-Wall -s -O2
override DESTDIR?=/usr/local

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
	$(CC) $(CFLAGS) $(@:.o=.c) -D__NAME=\"$(BIN)\" -c -o $@

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

# Build manpage
$(BIN).1: manpage.1.md
	env NAME=$(BIN) envsubst < manpage.1.md | pandoc --standalone --from markdown --to man -o $(BIN).1
	env NAME=$(BIN) envsubst < manpage.1.md | pandoc --standalone --from markdown --to html -o $(BIN).html

.PHONY: install
install: default
	install pmlag   $(DESTDIR)/bin
	install pmlag.1 $(DESTDIR)/share/man/man1

.PHONY: clean
clean:
	rm -f $(OBJ)
