CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./src
LDFLAGS = -lmicrohttpd -lsqlite3 -lcjson -pthread

SRC_DIR = src
BIN_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)
TARGET = $(BIN_DIR)/shortener

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
