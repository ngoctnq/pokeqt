#!/bin/bash

# Compile the main application
# Linking main.cu and cards.cu
echo "Compiling main..."
nvcc -std=c++20 -rdc=true -o main main.cu cards.cu -I.
# nvcc -std=c++20 -o main main.cu -I.

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. Running main..."
    ./main
else
    echo "Compilation failed."
    exit 1
fi
