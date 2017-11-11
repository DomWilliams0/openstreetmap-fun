OBJ        = obj
BIN        = bin
LIB        = lib
SRC        = src
PROTO      = proto
GEN        = gen

LIB_CFLAGS = -I$(LIB)/generic-c-hashmap -I$(LIB)/vec/src $(shell pkg-config --cflags 'libprotobuf-c >= 1.0.0')
CFLAGS     = -std=c11 -c -Wall -Wextra -Wpedantic -O1 -I$(SRC) $(LIB_CFLAGS) -D_GNU_SOURCE
LDFLAGS    = -Wall -L/usr/local/lib -L$(LIB) -lm $(shell pkg-config --libs 'libprotobuf-c >= 1.0.0')

CC         = gcc
TARGET     = $(BIN)/osm

RELEASE ?= 0
ifeq ($(RELEASE), 0)
	CFLAGS += -DDEBUG -O0 -g
endif

SRCS       := $(shell find $(SRC) -type f -name '*.c')
INCS       := $(shell find $(SRC) -type f -name '*.h')
PROTO_SRC  := $(shell find $(PROTO) -type f -name '*.proto')

LIB_SRCS   := lib/vec/src/vec.c
SRCS       := $(SRCS) $(LIB_SRCS)

OBJS       := $(addprefix $(OBJ)/,$(notdir $(SRCS:%.c=%.o)))
PROTO_OBJ  := $(addprefix $(OBJ)/,$(notdir $(PROTO_SRC:%.proto=%.pb-c.o)))
PROTO_GENS := $(addprefix $(GEN)/,$(notdir $(PROTO_SRC:%.proto=%.pb-c.c))) $(addprefix $(GEN)/,$(notdir $(PROTO_SRC:%.proto=%.pb-c.h)))

VPATH  = $(shell find $(SRC) $(LIB) -type d)

.PRECIOUS: $(PROTO_GENS)

.PHONY: default
default: run

.PHONY: bin
bin: $(TARGET)

.PHONY: pb
pb: $(PROTO_OBJ) | build_dirs

.PHONY: clean
clean:
	@rm -rf $(OBJ) $(BIN) $(GEN)

$(TARGET): $(OBJS) $(PROTO_OBJ)
	$(CC) $(LDFLAGS) $(OBJS) $(PROTO_OBJ) -o $@
	@echo "Compiled $(TARGET)"

# src -> obj
$(OBJS): $(OBJ)/%.o : %.c build_dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(GEN)/%.pb-c.c: $(PROTO)/%.proto
	protoc --c_out=$(GEN) -I $(PROTO) $<

$(OBJ)/%.pb-c.o: $(GEN)/%.pb-c.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: build_dirs
build_dirs:
	@mkdir -p $(BIN) $(OBJ) $(GEN)

.PHONY: run
run: $(TARGET)
	$(TARGET)

