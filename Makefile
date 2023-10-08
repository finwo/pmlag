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

build/${TARGET}/pmlag: build/${TARGET} $(wildcard build/${TARGET}/src/*.c build/${TARGET}/src/*.h build/${TARGET}/src/*/*.c build/${TARGET}/src/*/*.h)
	cd build/${TARGET} && dep install
	$(MAKE) --directory build/${TARGET} TARGET=${TARGET}

build/${TARGET}/pmlag-${SVC}-${TARGET}.tar.gz: build/${TARGET}/pmlag
	$(MAKE) --directory build/${TARGET} TARGET=${TARGET} SVC=${SVC} pmlag-${SVC}-${TARGET}.tar.gz

.PHONY: clean
clean:
	rm -rf build
