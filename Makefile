CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -g -Iinclude
LDFLAGS :=

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := simulador

.PHONY: all clean run compile_commands

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN) --modo single examples/processos.txt

compile_commands:
	@printf '[\n' > compile_commands.json
	@for file in $(SRC); do \
		printf '  {\n' >> compile_commands.json; \
		printf '    "directory": "%s",\n' "$$(pwd)" >> compile_commands.json; \
		printf '    "command": "$(CC) $(CFLAGS) -c %s -o %s",\n' "$$file" "$${file%.c}.o" >> compile_commands.json; \
		printf '    "file": "%s"\n' "$$file" >> compile_commands.json; \
		if [ "$$file" = "$$(printf "%s\n" $(SRC) | tail -n 1)" ]; then \
			printf '  }\n' >> compile_commands.json; \
		else \
			printf '  },\n' >> compile_commands.json; \
		fi; \
	done
	@printf ']\n' >> compile_commands.json

clean:
	rm -f $(OBJ) $(BIN)
