#!/bin/sh

echo "export CDE_INCLUDE_PATH=/opt/include" >> ~/.bashrc
echo "export CDE_LIBRARY_PATH=/opt/lib" >> ~/.bashrc
echo "export PATH=\$PATH:/opt/bin" >> ~/.bashrc
echo "Please reopen the terminal."
