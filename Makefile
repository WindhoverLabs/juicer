mkfile_path  := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
ROOT_DIR     := $(shell cd $(shell dirname $(mkfile_path)); pwd)
BUILD_DIR    := $(ROOT_DIR)/build
COVERAGE_DIR := $(BUILD_DIR)/coverage

# Target directories
SRC_DIR    := $(ROOT_DIR)/src
OBJ_DIR    := $(BUILD_DIR)/obj
BIN_DIR    := $(BUILD_DIR)
INCLUDES   := -I$(SRC_DIR) -I/usr/include/libdwarf -I$(ROOT_DIR)/CRCpp/inc

# External directories
CATCH2_DIR := $(ROOT_DIR)/Catch2

# Unit test directories
UT_SRC_DIR := $(ROOT_DIR)/unit-test
UT_OBJ_DIR := $(BUILD_DIR)/ut_obj
UT_OBJ_32BIT_DIR := $(BUILD_DIR)/ut_obj_32
UT_BIN_DIR := $(BUILD_DIR)
UT_INCLUDES := -I$(CATCH2_DIR)/single_include/catch2


# Target files
EXE        := $(BIN_DIR)/juicer
SRC        := $(wildcard $(SRC_DIR)/*.cpp)
OBJ        := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Unit test files
UT_EXE       := $(UT_BIN_DIR)/juicer-ut
UT_EXE_32BIT := $(UT_BIN_DIR)/juicer-ut-32
UT_SRC       := $(wildcard $(UT_SRC_DIR)/*.cpp) $(filter-out $(SRC_DIR)/main.cpp, $(SRC))
UT_OBJ       := $(UT_SRC:$(UT_SRC_DIR)/%.cpp=$(UT_OBJ_DIR)/%.o)
UT_OBJ       := $(UT_OBJ:$(SRC_DIR)/%.cpp=$(UT_OBJ_DIR)/%.o)

UT_SRC_32     := $(wildcard $(UT_SRC_DIR)/test_file*.cpp)
UT_OBJ_32     := $(UT_SRC_32:$(UT_SRC_DIR)/test_file%.cpp=$(UT_OBJ_32BIT_DIR)/test_file%.o)
UT_OBJ_32     := $(UT_OBJ_32:$(UT_SRC_DIR)/test_file%.cpp=$(UT_OBJ_32BIT_DIR)/test_file%.o)

# Set target flags
CPPFLAGS     := -MMD -MP -std=c++14 -fmessage-length=0 $(INCLUDES)
CFLAGS       := -Wall -g3
CFLAGS_32BIT := -Wall -g3 -m32
LDFLAGS      := -Llib
LDLIBS       := -lm -ldwarf -lsqlite3 -lelf -lcrypto

# Set unit test flags
UT_CPPFLAGS       := $(CPPFLAGS) $(UT_INCLUDES)
UT_CFLAGS         := $(CFLAGS) --coverage
UT_CFLAGS_32BIT   := $(CFLAGS_32BIT) --coverage
UT_LDFLAGS        := $(LDFLAGS)
UT_LDLIBS         := $(LDLIBS) -lgcov

# Set tools
CC          := g++
LD          := g++

.PHONY: all clean run-tests coverage docs

# Target recipes
$(EXE): $(OBJ)
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR): 
	mkdir -p $@

# Unit test recipes
$(UT_EXE): $(UT_OBJ)
	$(LD) $(UT_LDFLAGS) $^ $(UT_LDLIBS) -o $@

$(UT_EXE_32BIT): $(UT_OBJ_32)
$(UT_OBJ_DIR)/%.o: $(UT_SRC_DIR)/%.cpp | $(UT_OBJ_DIR)
	$(CC) $(UT_CPPFLAGS) $(UT_CFLAGS) -c $< -o $@

$(UT_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(UT_OBJ_DIR)
	$(CC) $(UT_CPPFLAGS) $(UT_CFLAGS) -c $< -o $@

$(UT_OBJ_32BIT_DIR)/test_file%.o: $(UT_SRC_DIR)/test_file%.cpp | $(UT_OBJ_32BIT_DIR)
	$(CC) $(UT_CPPFLAGS) $(UT_CFLAGS_32BIT) -c $< -o $@

$(UT_OBJ_DIR):
	mkdir -p $@


$(UT_OBJ_32BIT_DIR):
	mkdir -p $@

run-tests: $(UT_EXE_32BIT) | $(UT_EXE)
	-(cd $(BUILD_DIR); $(UT_EXE))
	

build-tests: | $(UT_EXE)

coverage: $(COVERAGE_DIR)/index.html

$(COVERAGE_DIR)/index.html: | run-tests
	mkdir -p $(COVERAGE_DIR)
	(cd $(COVERAGE_DIR); gcovr $(ROOT_DIR) --root $(ROOT_DIR) --object-directory $(UT_OBJ_DIR) --filter $(ROOT_DIR)/src/ --html --html-details -o index.html)


all: $(EXE) $(UT_EXE) $(UT_EXE_32BIT)

clean:
	@$(RM) -Rf $(BUILD_DIR)

-include $(UT_OBJ:.o=.d)
-include $(OBJ:.o=.d)

docker-build:
	@sudo docker build --no-cache -t juicer:latest -f Dockerfile .


check-format:
	@python3 clang_format_all.py --config clang_format_all_config.yaml

docs:
	@doxygen doxy_config

format:
	@python3 clang_format_all.py --config clang_format_all_config_format.yaml
