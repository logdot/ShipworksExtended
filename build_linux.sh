#!/bin/bash

# Download json.hpp if missing
if [ ! -f json.hpp ]; then
    echo "Downloading json.hpp..."
    wget -O json.hpp https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
fi

# Compile
# Uses stubs.cpp instead of shipworksASM.asm
# Statically links libgcc/libstdc++ to avoid Wine runtime conflicts
echo "Compiling ShipworksExtended.dll..."
x86_64-w64-mingw32-g++ -shared -o ShipworksExtended.dll src/stubs.cpp $(find src -name "*.cpp" ! -name "stubs.cpp") -static -static-libgcc -static-libstdc++ -I.

echo "Done."
