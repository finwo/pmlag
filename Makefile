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
	### linux-amd64 ###
	mkdir -p package/pmlag-linux-amd64
	cp    pmlag        package/pmlag-linux-amd64/pmlag
	cp    pmlag.1      package/pmlag-linux-amd64/pmlag.1
	cp -r service      package/pmlag-linux-amd64/service
	cp    Makefile.pkg package/pmlag-linux-amd64/Makefile
	(cd package ; tar c pmlag-linux-amd64 | gzip -9 > pmlag-linux-amd64.tar.gz)
	### openwrt-amd64 ###
	mkdir -p package/pmlag-openwrt-amd64/etc
	mkdir -p package/pmlag-openwrt-amd64/etc/init.d
	mkdir -p package/pmlag-openwrt-amd64/usr/bin
	mkdir -p package/pmlag-openwrt-amd64/usr/share/man/man1
	cp  pmlag                            package/pmlag-openwrt-amd64/usr/bin/pmlag
	cp  pmlag.1                          package/pmlag-openwrt-amd64/usr/share/man/man1/pmlag.1
	cp  service/procd/etc/init.d/pmlag   package/pmlag-openwrt-amd64/etc/init.d/pmlag
	cp  service/common/etc/pmlag.ini     package/pmlag-openwrt-amd64/etc/pmlag.ini
	(cd package ; tar c pmlag-openwrt-amd64 | gzip -9 > pmlag-openwrt-amd64.tar.gz)

.PHONY: clean
clean:
	rm -f $(OBJ)
