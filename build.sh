#!/bin/bash

echo ">>> Compilation du SERVEUR..."
gcc -std=c99 server/server_main.c -o blockblast_server.exe -lws2_32

if [ $? -eq 0 ]; then
    echo "✅ Serveur compilé avec succès !"
else
    echo "❌ Erreur de compilation du Serveur."
    exit 1
fi

echo "--------------------------------"

echo ">>> Compilation du CLIENT (Jeu)..."
# Note: On inclut les chemins SDL que nous avons utilisés précédemment
gcc -std=c99 \
    -I"C:/msys64/mingw64/include/SDL" \
    -L"C:/msys64/mingw64/lib" \
    client/main.c client/game.c client/net_client.c \
    -o blockblast.exe \
    -lmingw32 -lSDLmain -lSDL -lSDL_ttf -lws2_32 -mwindows

if [ $? -eq 0 ]; then
    echo "✅ Client compilé avec succès !"
    echo "--------------------------------"
    echo "🚀 Tout est prêt ! Tu peux lancer blockblast.exe ou blockblast_server.exe"
else
    echo "❌ Erreur de compilation du Client."
    exit 1
fi