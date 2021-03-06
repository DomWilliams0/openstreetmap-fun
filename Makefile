OBJ        = obj
BIN        = bin
LIB        = lib
SRC        = src
PROTO      = proto
GEN        = gen
TEST       = tests
BUILD_DIRS = $(BIN) $(OBJ) $(GEN)

LIB_CFLAGS = -I$(LIB)/generic-c-hashmap -I$(LIB)/vec/src -I$(LIB)/acutest/include
CFLAGS     = -std=c11 -Wall -Wextra -Wpedantic -O1 -I$(SRC) -I$(GEN) $(LIB_CFLAGS) -D_GNU_SOURCE
LDFLAGS    = -Wall -L$(BIN) -losm -L/usr/local/lib -L$(LIB) -lm

CC          = gcc
TARGET_LIB  = $(BIN)/libosm.a
TARGET_EXE  = $(BIN)/osm
TARGET_TEST = $(BIN)/osm_tests

RELEASE ?= 0
ifeq ($(RELEASE), 0)
	CFLAGS += -DDEBUG -O0 -g
endif

NO_PROTOBUF ?= 0
ifeq ($(NO_PROTOBUF),1)
	CFLAGS += -DNO_PROTOBUF
endif

INCS       := $(shell find $(SRC) -type f -name '*.h')
SRCS       := $(shell find $(SRC) -type f -name '*.c')
SRCS_TESTS := $(shell find $(TEST) -type f -name '*.c')
SRCS_LIB   := lib/vec/src/vec.c
SRC_MAIN   := $(SRC)/main.c

ifneq ($(NO_PROTOBUF),1)
	SRCS_PROTO := $(shell find $(PROTO) -type f -name '*.proto')
	SRCS_LIB   += lib/nanopb/pb_common.c lib/nanopb/pb_encode.c
else
	SRCS_PROTO :=
endif

SRCS       := $(filter-out $(SRC_MAIN),$(SRCS) $(SRCS_LIB))

OBJS       := $(addprefix $(OBJ)/,$(notdir $(SRCS:%.c=%.o)))
PROTO_OBJ  := $(addprefix $(OBJ)/,$(notdir $(SRCS_PROTO:%.proto=%.pb.o)))
PROTO_GENS := $(addprefix $(GEN)/,$(notdir $(SRCS_PROTO:%.proto=%.pb.c))) $(addprefix $(GEN)/,$(notdir $(PROTO_SRC:%.proto=%.pb.h)))

VPATH  = $(shell find $(SRC) $(LIB) $(TEST) -type d)

.PRECIOUS: $(PROTO_GENS)

.PHONY: default
default: run

.PHONY: exe
exe: $(TARGET_EXE)

.PHONY: test
test: $(TARGET_TEST)
	@$(TARGET_TEST)

.PHONY: pb
pb: $(PROTO_OBJ)

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
$(OBJS): $(OBJ)/%.o : %.c | $(OBJ) $(BIN)
	$(CC) $(CFLAGS) -c $< -o $@

$(GEN)/%.pb.c: $(PROTO)/%.proto
	$(MAKE) -C lib/nanopb/generator/proto
	protoc --plugin=protoc-gen-nanopb=lib/nanopb/generator/protoc-gen-nanopb --nanopb_out=$(GEN) -I $(PROTO) $<

$(OBJ)/%.pb.o: $(GEN)/%.pb.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIRS):
	@mkdir -p $(BUILD_DIRS)

.PHONY: run
run: $(TARGET_EXE)
	$(TARGET_EXE)

