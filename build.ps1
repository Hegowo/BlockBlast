param(
    [switch]$Embedded,
    [switch]$Clean
)

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "     SCRIPT DE COMPILATION BLOCKBLAST" -ForegroundColor Cyan
Write-Host "     (Edition Neon/Cyberpunk)" -ForegroundColor Magenta
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

if ($Embedded) {
    Write-Host "[MODE] AUTONOME (assets integres)" -ForegroundColor Yellow
} else {
    Write-Host "[MODE] STANDARD (assets externes)" -ForegroundColor White
}
Write-Host ""

if ($Clean -and (Test-Path "bin")) {
    Write-Host ">>> Nettoyage du dossier bin/..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "bin/*" -ErrorAction SilentlyContinue
}

if (!(Test-Path "bin")) {
    New-Item -ItemType Directory -Path "bin" | Out-Null
}

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

if ($Embedded) {
    Write-Host ">>> Generation des assets integres..." -ForegroundColor Yellow
    & powershell -ExecutionPolicy Bypass -File "embed_assets.ps1"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERREUR] Echec de la generation des assets integres" -ForegroundColor Red
        exit 1
    }
    Write-Host ""
}

Write-Host ">>> Compilation du SERVEUR..." -ForegroundColor Yellow
$serverResult = & gcc -std=c99 server/server_main.c -o bin/blockblast_server.exe -lws2_32 2>&1

if ($LASTEXITCODE -eq 0) {
    Write-Host "[OK] Serveur compile avec succes !" -ForegroundColor Green
} else {
    Write-Host "[ERREUR] Echec de la compilation du serveur :" -ForegroundColor Red
    Write-Host $serverResult
    exit 1
}

Write-Host ""

Write-Host ">>> Compilation du CLIENT..." -ForegroundColor Yellow

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

if ($Embedded) {
    $clientSources += "client/embedded_assets.c"
}

$clientArgs = @(
    "-std=c99"
    $SDL_INCLUDE
    $SDL_LIBPATH
)

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
    Write-Host "[OK] Client compile avec succes !" -ForegroundColor Green
} else {
    Write-Host "[ERREUR] Echec de la compilation du client :" -ForegroundColor Red
    Write-Host $clientResult
    exit 1
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "     COMPILATION TERMINEE !" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Executables crees dans le dossier bin/ :"
Write-Host "  - blockblast_server.exe"
Write-Host "  - blockblast.exe"
Write-Host ""

if ($Embedded) {
    Write-Host "VERSION AUTONOME :" -ForegroundColor Green
    Write-Host "  Tous les assets sont integres dans l'executable !" -ForegroundColor Green
    Write-Host "  Vous devez seulement distribuer :" -ForegroundColor White
    Write-Host "    - blockblast.exe" -ForegroundColor White
    Write-Host "    - blockblast_server.exe" -ForegroundColor White
    Write-Host "    - SDL2.dll, SDL2_ttf.dll, SDL2_mixer.dll" -ForegroundColor White
    Write-Host ""
    Write-Host "  Pas besoin du dossier assets/ !" -ForegroundColor Yellow
} else {
    Write-Host "Assets requis dans le dossier assets/ :" -ForegroundColor Yellow
    Write-Host "  - font.ttf (police de secours)"
    Write-Host "  - orbitron.ttf (police neon - telecharger depuis Google Fonts)"
    Write-Host ""
    Write-Host "ASTUCE : Lancez avec -Embedded pour une version autonome :" -ForegroundColor Cyan
    Write-Host "  .\build.ps1 -Embedded" -ForegroundColor White
}

Write-Host ""
Write-Host "Pour lancer :" -ForegroundColor Cyan
Write-Host "  1. Demarrer le serveur :  .\bin\blockblast_server.exe"
Write-Host "  2. Demarrer le client :   .\bin\blockblast.exe"
