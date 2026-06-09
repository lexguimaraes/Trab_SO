CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -g -Iinclude
LDFLAGS :=

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := simulador

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN) --modo single examples/processos.txt

clean:
	rm -f $(OBJ) $(BIN)
