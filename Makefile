.PHONY: all clean

SRC = src
FLEX = flex
BISON = bison
CPP = clang++
CPP_FLAGS = -std=c++17 -O3 -Wall -Wextra -Wno-unused-result

all: build/compiler
	cp build/compiler .
	echo "OK"

build/compiler: $(SRC)/main.cpp build/sysy.lex.cpp build/sysy.tab.cpp
	$(CPP) $(CPP_FLAGS) -o build/compiler $(SRC)/main.cpp build/sysy.lex.cpp build/sysy.tab.cpp 

build/sysy.lex.cpp: build $(SRC)/sysy.l $(SRC)/sysy.y
	$(FLEX) -o build/sysy.lex.cpp $(SRC)/sysy.l

build/sysy.tab.cpp: build $(SRC)/sysy.y
	$(BISON) -d -o build/sysy.tab.cpp $(SRC)/sysy.y

build:
	mkdir build

clean:
	rm -f compiler
	rm -rf build
