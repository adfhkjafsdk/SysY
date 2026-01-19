#!/bin/sh

./build/compiler -koopa test/1.sysy -o 1.koopa
./build/compiler -riscv test/1.sysy -o 1.S
