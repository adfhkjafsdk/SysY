.PHONY: all clean headers

# This Makefile is compatible with autotest
# autotest requirements:

SRC := lv4
FLEX := flex
BISON := bison
CPP := clang++
DEBUG := 0

CDE_LIBRARY_PATH ?= /opt/lib
CDE_INCLUDE_PATH ?= /opt/include

BUILD_DIR ?= build
LIB_DIR ?= $(CDE_LIBRARY_PATH)/native
INC_DIR ?= $(CDE_INCLUDE_PATH)

ifneq ($(DEBUG), 0)
  DEBUG_FLAG := -DYYDEBUG -g -fsanitize=undefined,address
  OPTIMIZE_FLAG := -O0
else
  DEBUG_FLAG := -DNDEBUG
  OPTIMIZE_FLAG := -O3
endif

CPP_FLAGS = -std=c++20 -Wall -Wextra -Wno-unused-result $(OPTIMIZE_FLAG) $(DEBUG_FLAG) -I$(INC_DIR) -L$(LIB_PATH) -lkoopa

all: $(BUILD_DIR)/compiler
	cp $(BUILD_DIR)/compiler .
	echo "OK"

$(BUILD_DIR)/compiler: headers $(SRC)/main.cpp $(SRC)/asmgen.cpp $(SRC)/irgen.cpp $(BUILD_DIR)/sysy.lex.cpp $(BUILD_DIR)/sysy.tab.cpp
	$(CPP) $(CPP_FLAGS) -o $(BUILD_DIR)/compiler $(SRC)/main.cpp $(SRC)/asmgen.cpp $(SRC)/irgen.cpp $(BUILD_DIR)/sysy.lex.cpp $(BUILD_DIR)/sysy.tab.cpp 

$(BUILD_DIR)/sysy.lex.cpp: $(BUILD_DIR) $(SRC)/sysy.l $(SRC)/sysy.y
	$(FLEX) -o $(BUILD_DIR)/sysy.lex.cpp $(SRC)/sysy.l

$(BUILD_DIR)/sysy.tab.cpp: $(BUILD_DIR) $(SRC)/sysy.y
	$(BISON) -d -o $(BUILD_DIR)/sysy.tab.cpp $(SRC)/sysy.y

headers: $(BUILD_DIR)/debug.hpp $(BUILD_DIR)/ast.hpp $(BUILD_DIR)/sysy_exceptions.hpp $(BUILD_DIR)/mir.hpp

$(BUILD_DIR)/debug.hpp: $(BUILD_DIR) $(SRC)/debug.hpp
	cp $(SRC)/debug.hpp $(BUILD_DIR)/debug.hpp

$(BUILD_DIR)/ast.hpp: $(BUILD_DIR) $(SRC)/ast.hpp
	cp $(SRC)/ast.hpp $(BUILD_DIR)/ast.hpp

$(BUILD_DIR)/sysy_exceptions.hpp: $(BUILD_DIR) $(SRC)/sysy_exceptions.hpp
	cp $(SRC)/sysy_exceptions.hpp $(BUILD_DIR)/sysy_exceptions.hpp

$(BUILD_DIR)/mir.hpp: $(BUILD_DIR) $(SRC)/mir.hpp
	cp $(SRC)/mir.hpp $(BUILD_DIR)/mir.hpp

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

clean:
	rm -f compiler
	rm -rf $(BUILD_DIR)
