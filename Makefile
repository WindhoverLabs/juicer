BUILD_DIR  := build

# Target directories
SRC_DIR    := src
OBJ_DIR    := $(BUILD_DIR)/obj
BIN_DIR    := $(BUILD_DIR)

# Unit test directories
UT_SRC_DIR := unit-test
UT_OBJ_DIR := $(BUILD_DIR)/ut_obj
UT_BIN_DIR := $(BUILD_DIR)

# External directories
CATCH2_DIR := Catch2

# Target files
EXE        := $(BIN_DIR)/juicer
SRC        := $(wildcard $(SRC_DIR)/*.cpp)
OBJ        := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Unit test files
UT_EXE     := $(UT_BIN_DIR)/juicer-ut
UT_SRC     := $(wildcard $(UT_SRC_DIR)/*.cpp) $(filter-out $(SRC_DIR)/main.cpp, $(SRC))
UT_OBJ     := $(UT_SRC:$(UT_SRC_DIR)/%.cpp=$(UT_OBJ_DIR)/%.o)

# Set target flags
CPPFLAGS    := -MMD -MP -std=c++14 -fmessage-length=0
CFLAGS      := -Wall -g
LDFLAGS     := -Llib
LDLIBS      := -lm -ldwarf -lsqlite3 -lelf

# Set unit test flags
UT_CPPFLAGS := $(CPPFLAGS) -I$(CATCH2_DIR)/single_include/catch2 -I$(SRC_DIR) --coverage
UT_CFLAGS   := $(CFLAGS)
UT_LDFLAGS  := $(LDFLAGS)
UT_LDLIBS   := $(LDLIBS) -lgcov

# Set compiler
CC          := g++

.PHONY: all clean run-tests

# Target recipes
$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR): 
	mkdir -p $@

# Unit test recipes
$(UT_EXE): $(UT_OBJ)
	$(CC) $(UT_LDFLAGS) $^ $(UT_LDLIBS) -o $@

$(UT_OBJ_DIR)/%.o: $(UT_SRC_DIR)/%.cpp $(UT_OBJ_DIR)
	$(CC) $(UT_CPPFLAGS) $(UT_CFLAGS) -c $< -o $@

$(UT_OBJ_DIR): 
	mkdir -p $@

run-tests: $(UT_EXE)
	$(UT_EXE)

#coverage: run_test
#	#lcov -c --directory $(GCOV_DIR) --output-file $(GCOV_DIR)/main_coverage.info
#	#lcov --remove $(JUICER_DIR)/../Catch2/single_include/catch2
#	
#	#genhtml $(GCOV_DIR)/main_coverage.info --output-directory $(GCOV_DIR)/report
#
#

all: $(EXE) $(UT_EXE)

clean:
	@$(RM) -Rf $(BUILD_DIR)

#-include $(OBJ:.o=.d)

