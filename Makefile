CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./src
LDFLAGS = -lmicrohttpd -lsqlite3 -lcjson -lcrypto -pthread

SRC_DIR = src
BIN_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)
HEADERS = $(wildcard $(SRC_DIR)/*.h)
TARGET = $(BIN_DIR)/shortener

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Recompile .c files when ANY header changes (safe for small projects)
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
