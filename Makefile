SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

default: pmlag

pmlag: $(OBJ)
	$(CC) $(SRC) -o pmlag

.PHONY: clean
clean:
	rm -rf $(OBJ)
