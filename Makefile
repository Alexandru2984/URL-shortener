CC ?= gcc

CPPFLAGS := -I./src -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_FORTIFY_SOURCE=3
CFLAGS := -std=c17 -O2 -g -Wall -Wextra -Werror -Wconversion -Wshadow -Wformat=2 \
          -Wstrict-prototypes -Werror=implicit-function-declaration -fstack-protector-strong -fPIE
LDFLAGS := -pie -Wl,-z,relro,-z,now
LDLIBS := -lmicrohttpd -lsqlite3 -lcjson -lcrypto -pthread
DEPFLAGS := -MMD -MP

SRC_DIR := src
BIN_DIR := bin
TEST_DIR := tests
BUILD_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)
TARGET := $(BIN_DIR)/shortener

TEST_UTILS := $(BUILD_DIR)/test_utils
TEST_DB := $(BUILD_DIR)/test_db
SANITIZE_TARGET := $(BIN_DIR)/shortener-asan

.PHONY: all clean run test check sanitize

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

$(TEST_UTILS): $(TEST_DIR)/test_utils.c $(SRC_DIR)/utils.c $(SRC_DIR)/utils.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(TEST_DIR)/test_utils.c $(SRC_DIR)/utils.c $(LDLIBS)

$(TEST_DB): $(TEST_DIR)/test_db.c $(SRC_DIR)/db.c $(SRC_DIR)/db.h $(SRC_DIR)/utils.c $(SRC_DIR)/utils.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(TEST_DIR)/test_db.c $(SRC_DIR)/db.c $(SRC_DIR)/utils.c $(LDLIBS)

test: $(TEST_UTILS) $(TEST_DB)
	$(TEST_UTILS)
	$(TEST_DB)

check: test
	cppcheck --enable=warning,style,performance,portability --std=c11 --suppress=missingIncludeSystem --check-level=exhaustive $(SRC_DIR)

$(SANITIZE_TARGET): $(SRCS) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) -std=c17 -O1 -g3 -Wall -Wextra -Werror -fno-omit-frame-pointer \
		-fsanitize=address,undefined -o $@ $(SRCS) $(LDLIBS) -fsanitize=address,undefined

sanitize: $(SANITIZE_TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(SRC_DIR)/*.o $(SRC_DIR)/*.d $(TARGET) $(SANITIZE_TARGET) $(TEST_UTILS) $(TEST_DB)
	rmdir $(BUILD_DIR) 2>/dev/null || true

-include $(DEPS)
