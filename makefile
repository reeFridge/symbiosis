.PHONY: all clean

CC = gcc
CFLAGS = -Wall -std=c11 -D_DEFAULT_SOURCE
LDLIBS = -lraylib -lGL -lm -lpthread -ldl -lrt
# On X11 requires also below libraries
LDLIBS += -lX11

SOURCES = src/main.c

BUILD_ROOT = ./build
OBJ_FILES = $(SOURCES:%.c=$(BUILD_ROOT)/%.o)
EXE = $(BUILD_ROOT)/main

all: $(EXE)

clean:
	rm -rf $(BUILD_ROOT)

$(EXE): $(OBJ_FILES)
	$(CC) -o $@ $^ $(LDLIBS)

$(OBJ_FILES): $(BUILD_ROOT)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<
