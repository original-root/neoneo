#!/bin/bash

# Script to build and run neoneo with debug symbols

# Set the build type to Debug
CMAKE_BUILD_TYPE=Debug

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}Building neoneo with debug symbols...${NC}"

# Create build directory if it doesn't exist
mkdir -p build

# Configure with debug symbols
cmake -B build -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE

# Build
if cmake --build build; then
    echo -e "${GREEN}Build successful!${NC}"
    
    # Get command line arguments if any
    ARGS="$@"
    
    # If no arguments are provided, add default debug arguments
    if [ -z "$ARGS" ]; then
        ARGS="-t -s -f"  # Enable tools, shell, and file operations
    fi
    
    echo -e "${BLUE}Running neoneo with arguments: $ARGS${NC}"
    echo -e "${BLUE}Press Ctrl+C to exit${NC}"
    
    # Run the debug build
    LLDB_DEBUGINFO_PROMPT="none" ./build/neoneo $ARGS
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi