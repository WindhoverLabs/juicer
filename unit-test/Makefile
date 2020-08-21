SRC_DIR := .
OBJ_DIR := obj
BIN_DIR := bin
GCOV_DIR := obj
JUICER_DIR := ../juicer

EXE := $(BIN_DIR)/test_juicer
SRC := $(wildcard $(SRC_DIR)/*.cpp)
JUICER_SRC := $(wildcard $(JUICER_DIR)/*.cpp)
JUICER_SRC :=  $(filter-out $(JUICER_DIR)/main.cpp, $(JUICER_SRC))

OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o) 

JUICER_OBJ := $(JUICER_SRC:$(JUICER_DIR)/%.cpp=$(GCOV_DIR)/%.o)


$(info    VAR is $(JUICER_SRC))

CPPFLAGS := -I. -I../Catch2/single_include/catch2 -I../juicer -MMD -MP -std=c++14 -fmessage-length=0 --coverage
CFLAGS   := -Wall -g
LDFLAGS  := -Llib
LDLIBS   := -lm -ldwarf -lsqlite3 -lelf -lgcov
CC       := g++

.PHONY: all clean

all: $(EXE)

$(EXE): $(JUICER_OBJ) $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(GCOV_DIR)/%.o: $(JUICER_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

run_test: $(EXE)
	$(EXE)

coverage: run_test
	lcov -c --directory $(GCOV_DIR) --output-file $(GCOV_DIR)/main_coverage.info
	#lcov --remove $(JUICER_DIR)/../Catch2/single_include/catch2
	
	#genhtml $(GCOV_DIR)/main_coverage.info --output-directory $(GCOV_DIR)/report


clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

-include $(OBJ:.o=.d)

