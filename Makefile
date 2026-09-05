TARGET:=linux-glibc-amd64

.PHONY: default
default: build/${TARGET}/pmlag

build/${TARGET}/pmlag: build/${TARGET} $(wildcard src/*.c src/*.h src/*/*.c src/*/*.h)
	cd build/${TARGET} && dep install
	$(MAKE) --directory build/${TARGET} TARGET=${TARGET}

build/${TARGET}: $(wildcard src/*.c src/*.h src/*/*.c src/*/*.h)
	mkdir -p build/${TARGET}
	cp -rT manpage.1.md      build/${TARGET}/manpage.1.md
	cp -rT src/              build/${TARGET}/src
	cp -rT target/common/    build/${TARGET}
	cp -rT target/${TARGET}/ build/${TARGET}
