CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -g -Iinclude
LDFLAGS :=

SRC := $(wildcard src/*.c)
HEADERS := $(wildcard include/*.h)
COMPILE_DB_FILES := $(SRC) $(HEADERS)
OBJ := $(SRC:.c=.o)
BIN := simulador

.PHONY: all clean run compile_commands

all: $(BIN)

$(BIN): | compile_commands

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN) --modo single examples/processos.txt

compile_commands:
	@printf '[\n' > compile_commands.json
	@first=1; \
	for file in $(COMPILE_DB_FILES); do \
		if [ $$first -eq 0 ]; then \
			printf ',\n' >> compile_commands.json; \
		fi; \
		first=0; \
		case "$$file" in \
			*.h) command='$(CC) $(CFLAGS) -x c-header -c '"$$file"' -o /tmp/'"$$(basename "$$file")"'.o' ;; \
			*) command='$(CC) $(CFLAGS) -c '"$$file"' -o '"$${file%.c}"'.o' ;; \
		esac; \
		printf '  {\n' >> compile_commands.json; \
		printf '    "directory": "%s",\n' "$$(pwd)" >> compile_commands.json; \
		printf '    "command": "%s",\n' "$$command" >> compile_commands.json; \
		printf '    "file": "%s"\n' "$$file" >> compile_commands.json; \
		printf '  }\n' >> compile_commands.json; \
	done
	@printf ']\n' >> compile_commands.json

clean:
	rm -f $(OBJ) $(BIN)
