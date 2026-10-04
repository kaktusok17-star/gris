CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra

BIN := gris
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(BIN)
test: $(BIN)
	./tests/run.sh
.PHONY: all install clean test