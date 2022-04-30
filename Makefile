# Setup core
SRC:=$(wildcard src/*.c)
LIBS=
CFLAGS?=
BIN=pmlag

# Include lib/argparse
SRC+=lib/argparse/argparse.c
CFLAGS+=-Ilib/argparse
LIBS+=lib/argparse

# Include lib/ini
SRC+=lib/inih/ini.c
CFLAGS+=-Ilib/inih
LIBS+=lib/inih

# Translate and default rule
OBJ:=$(SRC:.c=.o)
default: $(BIN) $(BIN).1 README.md

# How to fetch libraries
$(LIBS):
	git submodule update --init $@

# How to generate object files
$(OBJ): $(LIBS)
	$(CC) $(CFLAGS) $(@:.o=.c) -c -o $@

# Build main binary
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN)

# Build manpage
$(BIN).1: manpage.1.md
	env NAME=$(BIN) envsubst < manpage.1.md | pandoc --standalone --from markdown --to man -o $(BIN).1
	env NAME=$(BIN) envsubst < manpage.1.md | pandoc --standalone --from markdown --to html -o $(BIN).html

README.md: manpage.1.md
	env NAME=$(BIN) envsubst < manpage.1.md > README.md

# Cleanup
.PHONY: clean
clean:
	rm -rf $(OBJ)
