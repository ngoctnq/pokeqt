#!/bin/bash

# Compile the main application
echo "Compiling main..."
g++ -std=c++20 -o main main.cpp cards.cpp cards_dev.cpp -I.

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. Running main..."
    ./main
else
    echo "Compilation failed."
    exit 1
fi
