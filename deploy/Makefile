# Makefile for ChefsShell
# This compiles the C shell for web deployment

CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lreadline

# Source files
SRC = src/main.c
TARGET = chefs_shell

# Default target
all: $(TARGET)

$(TARGET): $(SRC)
	@echo "🔨 Compiling ChefsShell..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	@echo "✅ Compilation complete! Binary: ./$(TARGET)"
	@chmod +x $(TARGET)

clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -f $(TARGET)
	@echo "✅ Clean complete!"

rebuild: clean all

.PHONY: all clean rebuild
