BUILD = build
TARGET = $(BUILD)/llorch
CC = clang
CFLAGS = -Wall -Wextra -g -std=c17 -D_XOPEN_SOURCE=700 -Isrc
LDFLAGS = -lcurl -ljson-c

UI ?= cli

ifeq ($(UI), cli)
    LDFLAGS += -lreadline
endif

SRC = src
SRCS = $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/ui/$(UI)/*.c)
OBJS = $(SRCS:$(SRC)/%.c=$(BUILD)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) $(TARGET)

.PHONY: all clean $(TARGET)
