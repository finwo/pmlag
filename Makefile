TARGET:=linux-glibc-amd64
SVC:=generic

# # Initial config
# LIBS=
# SRC:=
# SRC+=$(wildcard src/*.c)
# SRC+=$(wildcard src/*/*.c)

# override CFLAGS?=-Wall -s -O2

# INCLUDES:=
# INCLUDES+=-I src

# include lib/.dep/config.mk

# override CFLAGS+=$(INCLUDES)

# # Which objects to generate before merging everything together
# OBJ:=$(SRC:.c=.o)

.PHONY: default
default: build/${TARGET}/pmlag

.PHONY: package
package: build/${TARGET}/pmlag-${SVC}-${TARGET}.tar.gz

build/${TARGET}: $(wildcard src/*.c src/*.h src/*/*.c src/*/*.h)
	mkdir -p build/${TARGET}
	cp     package.ini     build/${TARGET}/package.ini
	cp     manpage.1.md    build/${TARGET}/manpage.1.md
	cp -rT src/            build/${TARGET}/src
	cp -rT package/        build/${TARGET}/package
	cp -rT arch/common/    build/${TARGET}
	cp -rT arch/${TARGET}/ build/${TARGET}
	cp -rT arch/${TARGET}/ build/${TARGET}

build/${TARGET}/pmlag: build/${TARGET}
	cd build/${TARGET} && dep install
	$(MAKE) --directory build/${TARGET} TARGET=${TARGET}

build/${TARGET}/pmlag-${SVC}-${TARGET}.tar.gz: build/${TARGET}/pmlag
	$(MAKE) --directory build/${TARGET} TARGET=${TARGET} SVC=${SVC} pmlag-${SVC}-${TARGET}.tar.gz

# include Makefile.pkg

# %.o: %.c $(LIBS)
# 	$(CC) $(CFLAGS) $(@:.o=.c) -D__NAME=\"$(BIN)\" -c -o $@

# $(BIN): $(OBJ)
# 	$(CC) $(CFLAGS) $(OBJ) --static -o $@

# # Build manpage
# $(BIN).1: manpage.1.md
# 	env NAME=$(BIN) envsubst < manpage.1.md | pandoc --standalone --from markdown --to man -o $(BIN).1
# 	env NAME=$(BIN) envsubst < manpage.1.md | pandoc --standalone --from markdown --to html -o $(BIN).html

# package: default
# 	rm -rf package


# 	### openwrt-amd64 ###
# 	mkdir -p package/pmlag-openwrt-amd64/etc
# 	mkdir -p package/pmlag-openwrt-amd64/etc/init.d
# 	mkdir -p package/pmlag-openwrt-amd64/usr/bin
# 	mkdir -p package/pmlag-openwrt-amd64/usr/share/man/man1
# 	cp  pmlag                            package/pmlag-openwrt-amd64/usr/bin/pmlag
# 	cp  pmlag.1                          package/pmlag-openwrt-amd64/usr/share/man/man1/pmlag.1
# 	cp  service/procd/etc/init.d/pmlag   package/pmlag-openwrt-amd64/etc/init.d/pmlag
# 	cp  service/common/etc/pmlag.ini     package/pmlag-openwrt-amd64/etc/pmlag.ini
# 	(cd package ; tar c pmlag-openwrt-amd64 | gzip -9 > pmlag-openwrt-amd64.tar.gz)

.PHONY: clean
clean:
	rm -rf build
