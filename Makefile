# ================================================
# Cynex Makefile
# ================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99

# Release flags

RELEASE_FLAGS = -O3 -flto -Wl,-s -DNDEBUG

# Debug flags

DEBUG_FLAGS = -g -O0 -DDEBUG

IDIR = include
CFLAGS += -I$(IDIR)

SRC = src/main.c \
      src/repl.c \
      src/parser.c \
      src/lexer.c \
      src/variable.c \
      src/value.c \
      src/platform.c \
      src/CYNB/vm.c

OBJ = $(SRC:.c=.o)
TARGET = cynex

# Default = Release flags/build
all: CFLAGS += $(RELEASE_FLAGS)
all: $(TARGET)

# Debug flags/build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)
	@echo " Cynex building in DEBUG mode "

# Link step
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^
ifeq ($(OS),Windows_NT)
	@echo "Win build complete."
else
	-strip $@ 2>/dev/null || true   # strip symbols to make binary smaller
	@echo "Linux build complete."
endif

# Compile each file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) *.exe
	@echo "Cleaned."

run: all
	./$(TARGET)

.PHONY: all debug clean run
