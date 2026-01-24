.PHONY: all clean

# This Makefile is compatible with autotest
# autotest requirements:
#	The main target file should be named 'compiler' and located in the $(BUILD_DIR) directory.

SRC := lv6
FLEX := flex
BISON := bison
CPP := clang++
DEBUG ?= 0

CDE_LIBRARY_PATH ?= /opt/lib
CDE_INCLUDE_PATH ?= /opt/include

BUILD_DIR ?= build
LIB_DIR ?= $(CDE_LIBRARY_PATH)/native
INC_DIR ?= $(CDE_INCLUDE_PATH)

ifneq ($(DEBUG), 0)
  DEBUG_FLAG := -DYYDEBUG -g -fsanitize=undefined,address
  OPTIMIZE_FLAG := -O0
  LD_FLAGS := -L$(LIB_DIR) -lasan -lubsan -lkoopa 
else
  DEBUG_FLAG := -DNDEBUG
  OPTIMIZE_FLAG := -O3
  LD_FLAGS := -L$(LIB_DIR) -lkoopa
endif

CPP_FLAGS = -c -std=c++20 -Wall -Wextra -Wno-unused-result -Wno-unused-function $(OPTIMIZE_FLAG) $(DEBUG_FLAG) -I$(INC_DIR)

all: $(BUILD_DIR)/compiler
	cp $(BUILD_DIR)/compiler .
	echo "OK"

# $(BUILD_DIR)/compiler: headers $(SRC)/main.cpp $(SRC)/asmgen.cpp $(SRC)/irgen.cpp $(SRC)/ast.cpp $(BUILD_DIR)/sysy.lex.cpp $(BUILD_DIR)/sysy.tab.cpp
# 	$(CPP) $(CPP_FLAGS) -o $(BUILD_DIR)/compiler $(SRC)/main.cpp $(SRC)/asmgen.cpp $(SRC)/irgen.cpp $(SRC)/ast.cpp $(BUILD_DIR)/sysy.lex.cpp $(BUILD_DIR)/sysy.tab.cpp 

HEADERS_SRC = $(SRC)/debug.hpp $(SRC)/ast.hpp $(SRC)/sysy_exceptions.hpp $(SRC)/mir.hpp
HEADERS = $(BUILD_DIR)/debug.hpp $(BUILD_DIR)/ast.hpp $(BUILD_DIR)/sysy_exceptions.hpp $(BUILD_DIR)/mir.hpp
OBJS := $(BUILD_DIR)/sysy.lex.o $(BUILD_DIR)/sysy.tab.o $(BUILD_DIR)/ast.o $(BUILD_DIR)/irgen.o $(BUILD_DIR)/asmgen.o $(BUILD_DIR)/main.o

$(BUILD_DIR)/compiler: $(OBJS)
	$(CPP) $(OBJS) $(LD_FLAGS) -o $(BUILD_DIR)/compiler

$(BUILD_DIR)/sysy.lex.o: $(HEADERS) $(BUILD_DIR)/sysy.lex.cpp $(BUILD_DIR)/sysy.tab.hpp
	$(CPP) $(CPP_FLAGS) -o $(BUILD_DIR)/sysy.lex.o $(BUILD_DIR)/sysy.lex.cpp

$(BUILD_DIR)/sysy.tab.o: $(HEADERS) $(BUILD_DIR)/sysy.tab.cpp $(BUILD_DIR)/sysy.tab.hpp
	$(CPP) $(CPP_FLAGS) -o $(BUILD_DIR)/sysy.tab.o $(BUILD_DIR)/sysy.tab.cpp

$(BUILD_DIR)/ast.o: $(HEADERS_SRC) $(SRC)/ast.cpp
	$(CPP) $(CPP_FLAGS) -o $(BUILD_DIR)/ast.o $(SRC)/ast.cpp

$(BUILD_DIR)/irgen.o: $(HEADERS_SRC) $(SRC)/irgen.cpp
	$(CPP) $(CPP_FLAGS) -o $(BUILD_DIR)/irgen.o $(SRC)/irgen.cpp

$(BUILD_DIR)/asmgen.o: $(HEADERS_SRC) $(SRC)/asmgen.cpp
	$(CPP) $(CPP_FLAGS) -o $(BUILD_DIR)/asmgen.o $(SRC)/asmgen.cpp

$(BUILD_DIR)/main.o: $(HEADERS_SRC) $(SRC)/main.cpp
	$(CPP) $(CPP_FLAGS) -o $(BUILD_DIR)/main.o $(SRC)/main.cpp

$(BUILD_DIR)/sysy.lex.cpp: $(SRC)/sysy.l $(SRC)/sysy.y | $(BUILD_DIR)
	$(FLEX) -o $(BUILD_DIR)/sysy.lex.cpp $(SRC)/sysy.l

$(BUILD_DIR)/sysy.tab.hpp $(BUILD_DIR)/sysy.tab.cpp: $(SRC)/sysy.y | $(BUILD_DIR)
	$(BISON) -d -o $(BUILD_DIR)/sysy.tab.cpp $(SRC)/sysy.y 
# -Wcounterexamples

$(BUILD_DIR)/%.hpp: $(SRC)/%.hpp | $(BUILD_DIR)
	@if [ ! -f $@ ] || [ $< -nt $@ ]; then \
		echo "Copying $< to $@"; \
		cp $< $@; \
	fi

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

clean:
	rm -f compiler
	rm -rf $(BUILD_DIR)
