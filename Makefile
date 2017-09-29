OBJ        = obj
BIN        = bin
LIB        = lib
SRC        = src

LIB_CFLAGS = -I$(LIB)/generic-c-hashmap
CFLAGS     = -std=c11 -c -Wall -Wextra -O1 -I$(SRC) $(LIB_CFLAGS)
LDFLAGS    = -Wall -L/usr/local/lib -L$(LIB)

CC         = gcc
TARGET     = $(BIN)/osm

RELEASE ?= 0
ifeq ($(RELEASE), 0)
	CFLAGS += -DDEBUG -O0 -g
endif

SRCS  := $(shell find $(SRC) -type f -name '*.c')
INCS  := $(shell find $(SRC) -type f -name '*.h')

OBJS  := $(addprefix $(OBJ)/,$(notdir $(SRCS:%.c=%.o)))

VPATH  = $(shell find $(SRC) -type d)

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

