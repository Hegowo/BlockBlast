# BlockBlast Build Script for Windows PowerShell
# Run with: .\build.ps1

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "     BLOCKBLAST BUILD SCRIPT" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

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
$clientArgs = @(
    "-std=c99"
    $SDL_INCLUDE
    $SDL_LIBPATH
    "client/main.c"
    "client/game.c"
    "client/net_client.c"
    "-o"
    "bin/blockblast.exe"
    "-lmingw32"
    "-lSDLmain"
    "-lSDL"
    "-lSDL_ttf"
    "-lws2_32"
) | Where-Object { $_ -ne "" }

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
Write-Host "Make sure assets/font.ttf exists before running the client!" -ForegroundColor Yellow
Write-Host ""
Write-Host "To run:" -ForegroundColor Cyan
Write-Host "  1. Start the server:  .\bin\blockblast_server.exe"
Write-Host "  2. Start the client:  .\bin\blockblast.exe"

