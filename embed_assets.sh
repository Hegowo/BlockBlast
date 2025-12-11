#!/bin/bash

OUTPUT_HEADER="client/embedded_assets.h"
OUTPUT_SOURCE="client/embedded_assets.c"

declare -a ASSETS=(
    "font_orbitron:assets/orbitron.ttf"
    "font_default:assets/font.ttf"
    "sound_place:assets/sounds/place.wav"
    "sound_clear:assets/sounds/clear.wav"
    "sound_gameover:assets/sounds/gameover.wav"
    "sound_click:assets/sounds/click.wav"
    "sound_victory:assets/sounds/victory.wav"
    "sound_music:assets/sounds/music.wav"
)

echo "========================================"
echo "  BLOCKBLAST - Integrateur d'assets"
echo "========================================"
echo ""

BIN2C_EXE="tools/bin2c"
BIN2C_SRC="tools/bin2c.c"

if [ ! -d "tools" ]; then
    mkdir -p tools
fi

# Compiler bin2c si nécessaire
if [ ! -f "$BIN2C_EXE" ] || [ "$BIN2C_SRC" -nt "$BIN2C_EXE" ]; then
    echo "Compilation de l'outil bin2c..."
    if gcc -O2 "$BIN2C_SRC" -o "$BIN2C_EXE" 2>&1; then
        echo "[OK] bin2c compile"
    else
        echo "[ERREUR] Echec de la compilation de bin2c" >&2
        exit 1
    fi
fi

TEMP_DIR="tools/temp_assets"
if [ ! -d "$TEMP_DIR" ]; then
    mkdir -p "$TEMP_DIR"
fi

HEADER_CONTENT="#ifndef EMBEDDED_ASSETS_H
#define EMBEDDED_ASSETS_H

#include <stddef.h>

#ifdef EMBED_ASSETS

"

SOURCE_CONTENT="#include \"embedded_assets.h\"

#ifdef EMBED_ASSETS

"

TOTAL_SIZE=0

for ASSET_ENTRY in "${ASSETS[@]}"; do
    NAME="${ASSET_ENTRY%%:*}"
    ASSET_PATH="${ASSET_ENTRY#*:}"
    TEMP_FILE="$TEMP_DIR/${NAME}.c"
    
    if [ -f "$ASSET_PATH" ]; then
        FILE_SIZE=$(stat -c%s "$ASSET_PATH" 2>/dev/null || stat -f%z "$ASSET_PATH" 2>/dev/null || wc -c < "$ASSET_PATH" | tr -d ' ')
        if command -v bc >/dev/null 2>&1; then
            FILE_SIZE_KB=$(echo "scale=1; $FILE_SIZE / 1024" | bc)
        else
            FILE_SIZE_KB=$(awk "BEGIN {printf \"%.1f\", $FILE_SIZE / 1024}")
        fi
        
        echo -n "Conversion de $ASSET_PATH ($FILE_SIZE_KB Ko)... "
        
        if "$BIN2C_EXE" "$ASSET_PATH" "$TEMP_FILE" "$NAME" >/dev/null 2>&1; then
            echo "[OK]"
            
            SOURCE_CONTENT="${SOURCE_CONTENT}$(cat "$TEMP_FILE")
"
            
            HEADER_CONTENT="${HEADER_CONTENT}extern const unsigned char ${NAME}_data[];
extern const size_t ${NAME}_size;

"
            
            TOTAL_SIZE=$((TOTAL_SIZE + FILE_SIZE))
        else
            echo "[ECHEC]"
        fi
    else
        echo "[IGNORE] $ASSET_PATH (non trouve)"
        
        HEADER_CONTENT="${HEADER_CONTENT}extern const unsigned char ${NAME}_data[];
extern const size_t ${NAME}_size;

"
        
        SOURCE_CONTENT="${SOURCE_CONTENT}const unsigned char ${NAME}_data[] = { 0 };
const size_t ${NAME}_size = 0;

"
    fi
done

HEADER_CONTENT+="
#else

#define font_orbitron_data NULL
#define font_orbitron_size 0
#define font_default_data NULL
#define font_default_size 0
#define sound_place_data NULL
#define sound_place_size 0
#define sound_clear_data NULL
#define sound_clear_size 0
#define sound_gameover_data NULL
#define sound_gameover_size 0
#define sound_click_data NULL
#define sound_click_size 0
#define sound_victory_data NULL
#define sound_victory_size 0
#define sound_music_data NULL
#define sound_music_size 0

#endif

#endif
"

SOURCE_CONTENT+="
#endif
"

echo "$HEADER_CONTENT" > "$OUTPUT_HEADER"
echo "$SOURCE_CONTENT" > "$OUTPUT_SOURCE"

rm -rf "$TEMP_DIR"

echo ""
echo "[TERMINE] Fichiers generes :"
echo "  - $OUTPUT_HEADER"
echo "  - $OUTPUT_SOURCE"
echo ""

if command -v bc >/dev/null 2>&1; then
    TOTAL_SIZE_KB=$(echo "scale=2; $TOTAL_SIZE / 1024" | bc)
    TOTAL_SIZE_MB=$(echo "scale=2; $TOTAL_SIZE / 1024 / 1024" | bc)
else
    TOTAL_SIZE_KB=$(awk "BEGIN {printf \"%.2f\", $TOTAL_SIZE / 1024}")
    TOTAL_SIZE_MB=$(awk "BEGIN {printf \"%.2f\", $TOTAL_SIZE / 1024 / 1024}")
fi
echo "Taille totale des assets : $TOTAL_SIZE_KB Ko ($TOTAL_SIZE_MB Mo)"
echo ""

