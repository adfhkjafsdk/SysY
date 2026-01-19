#!/bin/sh

clang $1 -c -o $1.o -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
ld.lld $1.o -L$CDE_LIBRARY_PATH/riscv32 -lsysy -o a.out
echo "Compilation complete. Running ..."
qemu-riscv32-static a.out
