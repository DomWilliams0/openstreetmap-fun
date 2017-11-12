OBJ        = obj
BIN        = bin
LIB        = lib
SRC        = src
PROTO      = proto
GEN        = gen
TEST       = tests

LIB_CFLAGS = -I$(LIB)/generic-c-hashmap -I$(LIB)/vec/src $(shell pkg-config --cflags 'libprotobuf-c >= 1.0.0') -I$(LIB)/acutest/include
CFLAGS     = -std=c11 -Wall -Wextra -Wpedantic -O1 -I$(SRC) $(LIB_CFLAGS) -D_GNU_SOURCE
LDFLAGS    = -Wall -L$(BIN) -losm -L/usr/local/lib -L$(LIB) -lm $(shell pkg-config --libs 'libprotobuf-c >= 1.0.0')

CC          = gcc
TARGET_LIB  = $(BIN)/libosm.a
TARGET_EXE  = $(BIN)/osm
TARGET_TEST = $(BIN)/osm_tests

RELEASE ?= 0
ifeq ($(RELEASE), 0)
	CFLAGS += -DDEBUG -O0 -g
endif

INCS       := $(shell find $(SRC) -type f -name '*.h')
SRCS       := $(shell find $(SRC) -type f -name '*.c')
SRCS_PROTO := $(shell find $(PROTO) -type f -name '*.proto')
SRCS_TESTS := $(shell find $(TEST) -type f -name '*.c')
SRCS_LIB   := lib/vec/src/vec.c
SRC_MAIN   := $(SRC)/main.c
SRCS       := $(filter-out $(SRC_MAIN),$(SRCS) $(SRCS_LIB))

OBJS       := $(addprefix $(OBJ)/,$(notdir $(SRCS:%.c=%.o)))
PROTO_OBJ  := $(addprefix $(OBJ)/,$(notdir $(SRCS_PROTO:%.proto=%.pb-c.o)))
PROTO_GENS := $(addprefix $(GEN)/,$(notdir $(SRCS_PROTO:%.proto=%.pb-c.c))) $(addprefix $(GEN)/,$(notdir $(PROTO_SRC:%.proto=%.pb-c.h)))

VPATH  = $(shell find $(SRC) $(LIB) $(TEST) -type d)

.PRECIOUS: $(PROTO_GENS)

.PHONY: default
default: run

.PHONY: bin
bin: $(TARGET_EXE)

.PHONY: test
test: $(TARGET_TEST)
	@$(TARGET_TEST)

.PHONY: pb
pb: $(PROTO_OBJ) | build_dirs

.PHONY: clean
clean:
	@rm -rf $(OBJ) $(BIN) $(GEN)

$(TARGET_LIB): $(OBJS) $(PROTO_OBJ)
	ar rcs $@ $^
	@echo "Compiled static lib $@"

$(TARGET_EXE): $(TARGET_LIB) $(SRC_MAIN)
	$(CC) $(SRC_MAIN) $(CFLAGS) $(LDFLAGS) -o $@
	@echo "Compiled executable $@"

$(TARGET_TEST): $(TARGET_LIB) $(SRCS_TESTS)
	$(CC) $(SRCS_TESTS) $(CFLAGS) $(LDFLAGS) -o $@

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
run: $(TARGET_EXE)
	$(TARGET_EXE)

