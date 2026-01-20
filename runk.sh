#!/bin/sh

koopac $1 | llc --filetype=obj -o ~runk_tmp.o
clang ~runk_tmp.o -L$CDE_LIBRARY_PATH/native -lsysy -o a.out
./a.out
echo "Return: "$?
rm ~runk_tmp.o 
