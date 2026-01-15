.PHONY: all clean headers

SRC = src
FLEX = flex
BISON = bison
CPP = clang++
DEBUG = 0

ifeq ($(DEBUG), 1)
  NDEBUG_FLAG =
else
  NDEBUG_FLAG = -DNDEBUG
endif

CPP_FLAGS = -std=c++17 -O3 -Wall -Wextra -Wno-unused-result -lkoopa $(NDEBUG_FLAG)

all: build/compiler
	cp build/compiler .
	echo "OK"

build/compiler: $(SRC)/main.cpp build/sysy.lex.cpp build/sysy.tab.cpp
	$(CPP) $(CPP_FLAGS) -o build/compiler $(SRC)/main.cpp build/sysy.lex.cpp build/sysy.tab.cpp 

build/sysy.lex.cpp: build $(SRC)/sysy.l $(SRC)/sysy.y headers
	$(FLEX) -o build/sysy.lex.cpp $(SRC)/sysy.l

build/sysy.tab.cpp: build $(SRC)/sysy.y headers
	$(BISON) -d -o build/sysy.tab.cpp $(SRC)/sysy.y

build/debug.hpp: build headers
	cp $(SRC)/debug.hpp build/debug.hpp

build:
	mkdir build

headers: $(SRC)/debug.hpp $(SRC)/ast.hpp
	cp $(SRC)/debug.hpp build/debug.hpp
	cp $(SRC)/ast.hpp build/ast.hpp

clean:
	rm -f compiler
	rm -rf build
