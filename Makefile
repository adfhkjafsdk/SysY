.PHONY: all clean headers

# This Makefile is compatible with autotest
# autotest requirements:

SRC := lv2
FLEX := flex
BISON := bison
CPP := clang++
DEBUG := 0

CDE_LIBRARY_PATH ?= /opt/lib
CDE_INCLUDE_PATH ?= /opt/include

BUILD_DIR ?= build
LIB_DIR ?= $(CDE_LIBRARY_PATH)/native
INC_DIR ?= $(CDE_INCLUDE_PATH)

ifeq ($(DEBUG), 1)
  NDEBUG_FLAG := 
else
  NDEBUG_FLAG := -DNDEBUG
endif

CPP_FLAGS = -std=c++17 -O3 -Wall -Wextra -Wno-unused-result $(NDEBUG_FLAG) -I$(INC_DIR) -L$(LIB_PATH) -lkoopa

all: $(BUILD_DIR)/compiler
	cp $(BUILD_DIR)/compiler .
	echo "OK"

$(BUILD_DIR)/compiler: $(SRC)/main.cpp $(SRC)/asmgen.cpp $(BUILD_DIR)/sysy.lex.cpp $(BUILD_DIR)/sysy.tab.cpp
	$(CPP) $(CPP_FLAGS) -o $(BUILD_DIR)/compiler $(SRC)/main.cpp $(SRC)/asmgen.cpp $(BUILD_DIR)/sysy.lex.cpp $(BUILD_DIR)/sysy.tab.cpp 

$(BUILD_DIR)/sysy.lex.cpp: $(BUILD_DIR) $(SRC)/sysy.l $(SRC)/sysy.y headers
	$(FLEX) -o $(BUILD_DIR)/sysy.lex.cpp $(SRC)/sysy.l

$(BUILD_DIR)/sysy.tab.cpp: $(BUILD_DIR) $(SRC)/sysy.y headers
	$(BISON) -d -o $(BUILD_DIR)/sysy.tab.cpp $(SRC)/sysy.y

headers: $(BUILD_DIR)/debug.hpp $(BUILD_DIR)/ast.hpp $(BUILD_DIR)/sysy_exceptions.hpp $(BUILD_DIR)/mir.hpp

$(BUILD_DIR)/debug.hpp: $(SRC)/debug.hpp
	cp $(SRC)/debug.hpp $(BUILD_DIR)/debug.hpp

$(BUILD_DIR)/ast.hpp: $(SRC)/ast.hpp
	cp $(SRC)/ast.hpp $(BUILD_DIR)/ast.hpp

$(BUILD_DIR)/sysy_exceptions.hpp: $(SRC)/sysy_exceptions.hpp
	cp $(SRC)/sysy_exceptions.hpp $(BUILD_DIR)/sysy_exceptions.hpp

$(BUILD_DIR)/mir.hpp: $(SRC)/mir.hpp
	cp $(SRC)/mir.hpp $(BUILD_DIR)/mir.hpp

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

clean:
	rm -f compiler
	rm -rf $(BUILD_DIR)
