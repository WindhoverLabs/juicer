SRC_DIR := .
OBJ_DIR := obj
BIN_DIR := bin

EXE := $(BIN_DIR)/juicer
SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CPPFLAGS := -I. -MMD -MP -std=c++14 -ftest-coverage -fprofile-arcs  --coverage
CFLAGS   := -Wall
LDFLAGS  := -Llib
LDLIBS   := -lm -ldwarf -lsqlite3 -lelf -lgcov
CC       := g++

.PHONY: all clean 

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	@$(RM) -rvf $(BIN_DIR) $(OBJ_DIR) ./CodeCoverage

-include $(OBJ:.o=.d)

