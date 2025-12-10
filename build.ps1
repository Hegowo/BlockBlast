# BlockBlast Build Script for Windows PowerShell
# Run with: .\build.ps1
# Options:
#   -Embedded    : Build with all assets embedded (standalone exe)
#   -Clean       : Clean build directory before building

param(
    [switch]$Embedded,
    [switch]$Clean
)

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "     BLOCKBLAST BUILD SCRIPT" -ForegroundColor Cyan
Write-Host "     (Neon/Cyberpunk Edition)" -ForegroundColor Magenta
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

if ($Embedded) {
    Write-Host "[MODE] STANDALONE (assets embedded)" -ForegroundColor Yellow
} else {
    Write-Host "[MODE] STANDARD (external assets)" -ForegroundColor White
}
Write-Host ""

# Clean if requested
if ($Clean -and (Test-Path "bin")) {
    Write-Host ">>> Cleaning bin/ directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "bin/*" -ErrorAction SilentlyContinue
}

# Create output directory
if (!(Test-Path "bin")) {
    New-Item -ItemType Directory -Path "bin" | Out-Null
}

# Detect SDL path
$SDL_INCLUDE = ""
$SDL_LIBPATH = ""

if (Test-Path "C:/msys64/mingw64/include/SDL") {
    $SDL_INCLUDE = "-IC:/msys64/mingw64/include/SDL"
    $SDL_LIBPATH = "-LC:/msys64/mingw64/lib"
} elseif (Test-Path "C:/mingw64/include/SDL") {
    $SDL_INCLUDE = "-IC:/mingw64/include/SDL"
    $SDL_LIBPATH = "-LC:/mingw64/lib"
}

Write-Host "SDL Include: $SDL_INCLUDE"
Write-Host ""

# Generate embedded assets if needed
if ($Embedded) {
    Write-Host ">>> Generating embedded assets..." -ForegroundColor Yellow
    & powershell -ExecutionPolicy Bypass -File "embed_assets.ps1"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Failed to generate embedded assets" -ForegroundColor Red
        exit 1
    }
    Write-Host ""
}

# Compile Server
Write-Host ">>> Compiling SERVER..." -ForegroundColor Yellow
$serverResult = & gcc -std=c99 server/server_main.c -o bin/blockblast_server.exe -lws2_32 2>&1

if ($LASTEXITCODE -eq 0) {
    Write-Host "[OK] Server compiled successfully!" -ForegroundColor Green
} else {
    Write-Host "[ERROR] Server compilation failed:" -ForegroundColor Red
    Write-Host $serverResult
    exit 1
}

Write-Host ""

# Compile Client
Write-Host ">>> Compiling CLIENT..." -ForegroundColor Yellow

$clientSources = @(
    "client/main.c"
    "client/globals.c"
    "client/save_system.c"
    "client/audio.c"
    "client/graphics.c"
    "client/ui_components.c"
    "client/screens.c"
    "client/input_handlers.c"
    "client/game.c"
    "client/net_client.c"
)

# Add embedded assets source if in embedded mode
if ($Embedded) {
    $clientSources += "client/embedded_assets.c"
}

$clientArgs = @(
    "-std=c99"
    $SDL_INCLUDE
    $SDL_LIBPATH
)

# Add EMBED_ASSETS define if in embedded mode
if ($Embedded) {
    $clientArgs += "-DEMBED_ASSETS"
}

$clientArgs += $clientSources
$clientArgs += @(
    "-o"
    "bin/blockblast.exe"
    "-lmingw32"
    "-lSDLmain"
    "-lSDL"
    "-lSDL_ttf"
    "-lSDL_image"
    "-lSDL_mixer"
    "-lws2_32"
    "-lm"
    "-mwindows"
)

$clientArgs = $clientArgs | Where-Object { $_ -ne "" }

$clientResult = & gcc @clientArgs 2>&1

if ($LASTEXITCODE -eq 0) {
    Write-Host "[OK] Client compiled successfully!" -ForegroundColor Green
} else {
    Write-Host "[ERROR] Client compilation failed:" -ForegroundColor Red
    Write-Host $clientResult
    exit 1
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "     BUILD COMPLETE!" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Executables created in bin/ directory:"
Write-Host "  - blockblast_server.exe"
Write-Host "  - blockblast.exe"
Write-Host ""

if ($Embedded) {
    Write-Host "STANDALONE BUILD:" -ForegroundColor Green
    Write-Host "  All assets are embedded in the executable!" -ForegroundColor Green
    Write-Host "  You only need to distribute:" -ForegroundColor White
    Write-Host "    - blockblast.exe" -ForegroundColor White
    Write-Host "    - blockblast_server.exe" -ForegroundColor White
    Write-Host "    - SDL2.dll, SDL2_ttf.dll, SDL2_mixer.dll" -ForegroundColor White
    Write-Host ""
    Write-Host "  No assets/ folder needed!" -ForegroundColor Yellow
} else {
    Write-Host "Required assets in assets/ directory:" -ForegroundColor Yellow
    Write-Host "  - font.ttf (fallback font)"
    Write-Host "  - orbitron.ttf (neon font - download from Google Fonts)"
    Write-Host ""
    Write-Host "TIP: Run with -Embedded flag for standalone build:" -ForegroundColor Cyan
    Write-Host "  .\build.ps1 -Embedded" -ForegroundColor White
}

Write-Host ""
Write-Host "To run:" -ForegroundColor Cyan
Write-Host "  1. Start the server:  .\bin\blockblast_server.exe"
Write-Host "  2. Start the client:  .\bin\blockblast.exe"
