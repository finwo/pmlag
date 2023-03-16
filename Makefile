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
default: $(BIN) $(BIN).1 README.md

include Makefile.pkg

%.o: %.c $(LIBS)
	$(CC) $(CFLAGS) $(@:.o=.c) -D__NAME=\"$(BIN)\" -c -o $@

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

# Build manpage
$(BIN).1: manpage.1.md
	env NAME=$(BIN) envsubst < manpage.1.md | pandoc --standalone --from markdown --to man -o $(BIN).1
	env NAME=$(BIN) envsubst < manpage.1.md | pandoc --standalone --from markdown --to html -o $(BIN).html

package: default
	rm -rf package
	mkdir -p package/pmlag
	cp    pmlag        package/pmlag/pmlag
	cp    pmlag.1      package/pmlag/pmlag.1
	cp -r service      package/pmlag/service
	cp    Makefile.pkg package/pmlag/Makefile
	(cd package ; tar c pmlag | gzip -9 > pmlag-linux-amd64.tar.gz)

.PHONY: clean
clean:
	rm -f $(OBJ)
