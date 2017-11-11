OBJ        = obj
BIN        = bin
LIB        = lib
SRC        = src

LIB_CFLAGS = -I$(LIB)/generic-c-hashmap -I$(LIB)/vec/src
CFLAGS     = -std=c11 -c -Wall -Wextra -Wpedantic -O1 -I$(SRC) $(LIB_CFLAGS) -D_GNU_SOURCE
LDFLAGS    = -Wall -L/usr/local/lib -L$(LIB) -lm

CC         = gcc
TARGET     = $(BIN)/osm

RELEASE ?= 0
ifeq ($(RELEASE), 0)
	CFLAGS += -DDEBUG -O0 -g
endif

SRCS     := $(shell find $(SRC) -type f -name '*.c')
INCS     := $(shell find $(SRC) -type f -name '*.h')

LIB_SRCS := lib/vec/src/vec.c
SRCS     := $(SRCS) $(LIB_SRCS)

OBJS  := $(addprefix $(OBJ)/,$(notdir $(SRCS:%.c=%.o)))

VPATH  = $(shell find $(SRC) $(LIB) -type d)

.PHONY: default
default: run

.PHONY: clean
clean:
	@rm -rf $(OBJ) $(BIN)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@
	@echo "Compiled $(TARGET)"

# src -> obj
$(OBJS): $(OBJ)/%.o : %.c build_dirs
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: build_dirs
build_dirs:
	@mkdir -p $(BIN) $(OBJ)

.PHONY: run
run: $(TARGET)
	$(TARGET)

