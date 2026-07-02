CC = gcc
CFLAGS = -Wall -Wextra -O2 -lm -pthread
SOURCES = lexer.c parser.c compiler.c vm.c error.c sql.c visual.c jit.c db.c parallel.c gpu.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = emarald

EMERPM = emerpm

.PHONY: emerpm
emerpm: emerpm.c
	$(CC) $(CFLAGS) emerpm.c -o $(EMERPM)

WASM_TARGET = wasm/wasm_target

.PHONY: wasm-target
wasm-target: wasm/wasm_target.c
	$(CC) $(CFLAGS) wasm/wasm_target.c -o $(WASM_TARGET)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(CFLAGS)

%.o: %.c interpreter.h error.h sql.h visual.h jit.h db.h parallel.h gpu.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

help:
	@echo "Emarald Language - Build Commands:"
	@echo "  make        - Build the emarald interpreter"
	@echo "  make clean  - Remove compiled files"
	@echo "  make run    - Build and run the interpreter"
	@echo "  make help   - Show this help message"
