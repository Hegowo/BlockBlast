#!/bin/bash

echo "========================================="
echo "     SCRIPT DE COMPILATION BLOCKBLAST"
echo "     (Edition Neon/Cyberpunk)"
echo "========================================="

if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "mingw"* ]] || [[ "$OSTYPE" == "cygwin"* ]] || [[ -n "$WINDIR" ]]; then
    OS="windows"
    EXE_EXT=".exe"
    SOCKET_LIB="-lws2_32"
    SDL_FLAGS="-lmingw32 -lSDLmain -lSDL -lSDL_ttf -lSDL_image -lSDL_mixer"
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

echo "Systeme detecte : $OS"
echo "SDL Include : $SDL_INCLUDE"
echo "SDL LibPath : $SDL_LIBPATH"
echo ""

mkdir -p bin

echo ">>> Compilation du SERVEUR..."
SERVER_OUTPUT=$(gcc -std=c99 -Wall -Wextra \
    server/server_main.c \
    -o bin/blockblast_server${EXE_EXT} \
    $SOCKET_LIB 2>&1)
SERVER_RESULT=$?

if [ $SERVER_RESULT -eq 0 ]; then
    echo "[OK] Serveur compile avec succes !"
    if [ -n "$SERVER_OUTPUT" ]; then
        echo "Avertissements :"
        echo "$SERVER_OUTPUT"
    fi
else
    echo "[ERREUR] Echec de la compilation du serveur."
    echo "----------------------------------------"
    echo "$SERVER_OUTPUT"
    echo "----------------------------------------"
    exit 1
fi

echo ""

echo ">>> Compilation du CLIENT..."
CLIENT_OUTPUT=$(gcc -std=c99 -Wall -Wextra \
    $SDL_INCLUDE \
    $SDL_LIBPATH \
    client/main.c \
    client/globals.c \
    client/save_system.c \
    client/audio.c \
    client/graphics.c \
    client/ui_components.c \
    client/screens.c \
    client/input_handlers.c \
    client/game.c \
    client/net_client.c \
    -o bin/blockblast${EXE_EXT} \
    $SDL_FLAGS $SOCKET_LIB -lm 2>&1)
CLIENT_RESULT=$?

if [ $CLIENT_RESULT -eq 0 ]; then
    echo "[OK] Client compile avec succes !"
    if [ -n "$CLIENT_OUTPUT" ]; then
        echo "Avertissements :"
        echo "$CLIENT_OUTPUT"
    fi
else
    echo "[ERREUR] Echec de la compilation du client."
    echo "----------------------------------------"
    echo "$CLIENT_OUTPUT"
    echo "----------------------------------------"
    exit 1
fi

echo ""
echo "========================================="
echo "     COMPILATION TERMINEE !"
echo "========================================="
echo ""
echo "Executables crees dans le dossier bin/ :"
echo "  - blockblast_server${EXE_EXT}"
echo "  - blockblast${EXE_EXT}"
echo ""
echo "Assets requis dans le dossier assets/ :"
echo "  - font.ttf (police de secours)"
echo "  - orbitron.ttf (police neon - telecharger depuis Google Fonts)"
echo ""
echo "Pour lancer :"
echo "  1. Demarrer le serveur :  ./bin/blockblast_server${EXE_EXT}"
echo "  2. Demarrer le client :   ./bin/blockblast${EXE_EXT}"
