TARGET:=linux-glibc-amd64

.PHONY: default
default: build/${TARGET}/pmlag

FIND:=$(shell command -v gfind find)

WATCH?=
WATCH+=$(shell $(FIND) src -type f -name '*.c')
WATCH+=$(shell $(FIND) src -type f -name '*.h')

build/${TARGET}/pmlag: build/${TARGET} $(WATCH)
	cd build/${TARGET} && dep install
	$(MAKE) --directory build/${TARGET} TARGET=${TARGET}

build/${TARGET}: $(WATCH)
	mkdir -p build/${TARGET}
	cp -rT manpage.1.md      build/${TARGET}/manpage.1.md
	cp -rT src/              build/${TARGET}/src
	cp -rT target/common/    build/${TARGET}
	cp -rT target/${TARGET}/ build/${TARGET}
