#!/bin/bash

# BlockBlast Build Script
# Supports both Windows (MinGW/MSYS2) and Linux
# Neon/Cyberpunk Edition

echo "========================================="
echo "     BLOCKBLAST BUILD SCRIPT"
echo "     (Neon/Cyberpunk Edition)"
echo "========================================="

# Detect OS
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "mingw"* ]] || [[ "$OSTYPE" == "cygwin"* ]] || [[ -n "$WINDIR" ]]; then
    OS="windows"
    EXE_EXT=".exe"
    SOCKET_LIB="-lws2_32"
    SDL_FLAGS="-lmingw32 -lSDLmain -lSDL -lSDL_ttf -lSDL_image -lSDL_mixer"
    # Try to detect MSYS2 MinGW64 paths
    if [ -d "/mingw64/include/SDL" ]; then
        SDL_INCLUDE="-I/mingw64/include/SDL"
        SDL_LIBPATH="-L/mingw64/lib"
    elif [ -d "C:/msys64/mingw64/include/SDL" ]; then
        SDL_INCLUDE="-IC:/msys64/mingw64/include/SDL"
        SDL_LIBPATH="-LC:/msys64/mingw64/lib"
    else
        SDL_INCLUDE=""
        SDL_LIBPATH=""
    fi
else
    OS="linux"
    EXE_EXT=""
    SOCKET_LIB=""
    SDL_FLAGS="-lSDL -lSDL_ttf -lSDL_image -lSDL_mixer -lm"
    SDL_INCLUDE="$(sdl-config --cflags 2>/dev/null || echo '')"
    SDL_LIBPATH=""
fi

echo "Detected OS: $OS"
echo "SDL Include: $SDL_INCLUDE"
echo "SDL LibPath: $SDL_LIBPATH"
echo ""

# Create output directory
mkdir -p bin

# Compile Server
echo ">>> Compiling SERVER..."
SERVER_OUTPUT=$(gcc -std=c99 -Wall -Wextra \
    server/server_main.c \
    -o bin/blockblast_server${EXE_EXT} \
    $SOCKET_LIB 2>&1)
SERVER_RESULT=$?

if [ $SERVER_RESULT -eq 0 ]; then
    echo "[OK] Server compiled successfully!"
    if [ -n "$SERVER_OUTPUT" ]; then
        echo "Warnings:"
        echo "$SERVER_OUTPUT"
    fi
else
    echo "[ERROR] Server compilation failed."
    echo "----------------------------------------"
    echo "$SERVER_OUTPUT"
    echo "----------------------------------------"
    exit 1
fi

echo ""

# Compile Client (with SDL_image support)
echo ">>> Compiling CLIENT..."
CLIENT_OUTPUT=$(gcc -std=c99 -Wall -Wextra \
    $SDL_INCLUDE \
    $SDL_LIBPATH \
    client/main.c client/game.c client/net_client.c \
    -o bin/blockblast${EXE_EXT} \
    $SDL_FLAGS $SOCKET_LIB -lm 2>&1)
CLIENT_RESULT=$?

if [ $CLIENT_RESULT -eq 0 ]; then
    echo "[OK] Client compiled successfully!"
    if [ -n "$CLIENT_OUTPUT" ]; then
        echo "Warnings:"
        echo "$CLIENT_OUTPUT"
    fi
else
    echo "[ERROR] Client compilation failed."
    echo "----------------------------------------"
    echo "$CLIENT_OUTPUT"
    echo "----------------------------------------"
    exit 1
fi

echo ""
echo "========================================="
echo "     BUILD COMPLETE!"
echo "========================================="
echo ""
echo "Executables created in bin/ directory:"
echo "  - blockblast_server${EXE_EXT}"
echo "  - blockblast${EXE_EXT}"
echo ""
echo "Required assets in assets/ directory:"
echo "  - font.ttf (fallback font)"
echo "  - orbitron.ttf (neon font - download from Google Fonts)"
echo ""
echo "To run:"
echo "  1. Start the server:  ./bin/blockblast_server${EXE_EXT}"
echo "  2. Start the client:  ./bin/blockblast${EXE_EXT}"
