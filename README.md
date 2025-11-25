# BlockBlast V4.0

A block placement puzzle game written in C99 with SDL 1.2, featuring solo mode and online multiplayer.

## Features

- **Solo Mode**: Classic block placement puzzle with scoring
- **Online Multiplayer**: Turn-based gameplay on a shared grid with lobby system
- **Cross-platform**: Works on Windows (MinGW/Winsock) and Linux (GCC/BSD Sockets)
- **Custom UI**: Hand-built interface with buttons, text input fields, and mobile-style graphics

## Requirements

### Windows (MSYS2/MinGW64)

```bash
pacman -S mingw-w64-x86_64-SDL mingw-w64-x86_64-SDL_ttf mingw-w64-x86_64-gcc
```

### Linux (Debian/Ubuntu)

```bash
sudo apt-get install libsdl1.2-dev libsdl-ttf2.0-dev gcc
```

### Linux (Fedora)

```bash
sudo dnf install SDL-devel SDL_ttf-devel gcc
```

## Font Setup

You need to provide a TTF font file at `assets/font.ttf`. Any TTF font will work:

- [DejaVu Sans](https://dejavu-fonts.github.io/)
- [Roboto](https://fonts.google.com/specimen/Roboto)
- [Open Sans](https://fonts.google.com/specimen/Open+Sans)

```bash
mkdir -p assets
# Copy your font file to assets/font.ttf
```

## Building

### Using the Build Script

```bash
chmod +x build.sh
./build.sh
```

### Manual Compilation

#### Windows (MSYS2)

```bash
# Server
gcc -std=c99 server/server_main.c -o bin/blockblast_server.exe -lws2_32

# Client
gcc -std=c99 -I/mingw64/include/SDL -L/mingw64/lib \
    client/main.c client/game.c client/net_client.c \
    -o bin/blockblast.exe \
    -lmingw32 -lSDLmain -lSDL -lSDL_ttf -lws2_32
```

#### Linux

```bash
# Server
gcc -std=c99 server/server_main.c -o bin/blockblast_server

# Client
gcc -std=c99 $(sdl-config --cflags) \
    client/main.c client/game.c client/net_client.c \
    -o bin/blockblast \
    -lSDL -lSDL_ttf
```

## Running

### Start the Server

```bash
./bin/blockblast_server
```

The server listens on port 5000 by default.

### Start the Client

```bash
./bin/blockblast
```

## Gameplay

### Solo Mode

1. Click "SOLO" from the main menu
2. Drag pieces from the bottom to place them on the grid
3. Complete rows or columns to clear them and earn points
4. Game ends when no valid moves remain

### Multiplayer Mode

1. Click "MULTI ONLINE" to connect to the server
2. Enter your username
3. Create a room or join with a 4-character code
4. Host can start the game when 2+ players are present
5. Take turns placing pieces on the shared grid
6. Scores are saved to the server leaderboard

## Controls

- **Left Click**: Select and place pieces, click buttons
- **Right Click**: Kick players (host only in lobby)
- **Escape**: Return to previous menu

## Configuration

In the OPTIONS menu, you can configure:
- Server IP address (default: 127.0.0.1)
- Server port (default: 5000)

Test the connection before saving.

## Project Structure

```
BlockBlast/
├── assets/
│   └── font.ttf              # TTF font file (user provided)
├── client/
│   ├── main.c                # Client entry point, UI, game loop
│   ├── game.c/game.h         # Game logic (grid, pieces)
│   └── net_client.c/net_client.h  # Client socket management
├── server/
│   └── server_main.c         # Server logic, room management
├── common/
│   ├── config.h              # Shared constants
│   └── net_protocol.h        # Network packet definitions
├── build.sh                  # Build script
└── README.md                 # This file
```

## Technical Details

- **Language**: C99 standard
- **Graphics**: SDL 1.2 with SDL_ttf for text rendering
- **Networking**: TCP sockets with binary protocol
- **Grid**: 10x10 cells, 40 pixels per cell
- **Window**: 540x960 pixels (mobile-style portrait)

## Piece Types

| Shape | Size | Color |
|-------|------|-------|
| Single block | 1x1 | Red |
| Horizontal bar | 2x1 | Green |
| Long bar | 3x1 | Blue |
| Square | 2x2 | Yellow |
| T-shape | 2x3 | Magenta |

## Scoring

- Place a piece: +10 points
- Clear a row: +100 points
- Clear a column: +100 points

## License

This project is provided as-is for educational purposes.

