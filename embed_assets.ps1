$OutputHeader = "client/embedded_assets.h"
$OutputSource = "client/embedded_assets.c"

$Assets = @(
    @{ Name = "font_orbitron"; Path = "assets/orbitron.ttf" },
    @{ Name = "font_default"; Path = "assets/font.ttf" },
    @{ Name = "sound_place"; Path = "assets/sounds/place.wav" },
    @{ Name = "sound_clear"; Path = "assets/sounds/clear.wav" },
    @{ Name = "sound_gameover"; Path = "assets/sounds/gameover.wav" },
    @{ Name = "sound_click"; Path = "assets/sounds/click.wav" },
    @{ Name = "sound_victory"; Path = "assets/sounds/victory.wav" },
    @{ Name = "sound_music"; Path = "assets/sounds/music.wav" }
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  BLOCKBLAST - Asset Embedder" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$bin2cExe = "tools/bin2c.exe"
$bin2cSrc = "tools/bin2c.c"

if (!(Test-Path "tools")) {
    New-Item -ItemType Directory -Path "tools" | Out-Null
}

if (!(Test-Path $bin2cExe) -or ((Get-Item $bin2cSrc -ErrorAction SilentlyContinue).LastWriteTime -gt (Get-Item $bin2cExe -ErrorAction SilentlyContinue).LastWriteTime)) {
    Write-Host "Compiling bin2c tool..." -ForegroundColor Yellow
    $result = & gcc -O2 $bin2cSrc -o $bin2cExe 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Failed to compile bin2c: $result" -ForegroundColor Red
        exit 1
    }
    Write-Host "[OK] bin2c compiled" -ForegroundColor Green
}

$TempDir = "tools/temp_assets"
if (!(Test-Path $TempDir)) {
    New-Item -ItemType Directory -Path $TempDir | Out-Null
}

$HeaderContent = @"
#ifndef EMBEDDED_ASSETS_H
#define EMBEDDED_ASSETS_H

#include <stddef.h>

#ifdef EMBED_ASSETS

"@

$SourceContent = @"
#include "embedded_assets.h"

#ifdef EMBED_ASSETS

"@

$TotalSize = 0

foreach ($Asset in $Assets) {
    $Name = $Asset.Name
    $Path = $Asset.Path
    $TempFile = "$TempDir/${Name}.c"
    
    if (Test-Path $Path) {
        $FileSize = (Get-Item $Path).Length
        $FileSizeKB = [math]::Round($FileSize / 1024, 1)
        
        Write-Host -NoNewline "Converting $Path ($FileSizeKB KB)... "
        
        $result = & $bin2cExe $Path $TempFile $Name 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[OK]" -ForegroundColor Green
            
            $GeneratedContent = Get-Content $TempFile -Raw
            $SourceContent += $GeneratedContent + "`n"
            
            $HeaderContent += "extern const unsigned char ${Name}_data[];`n"
            $HeaderContent += "extern const size_t ${Name}_size;`n`n"
            
            $TotalSize += $FileSize
        } else {
            Write-Host "[FAILED]" -ForegroundColor Red
            Write-Host $result
        }
    }
    else {
        Write-Host "[SKIP] $Path (not found)" -ForegroundColor Yellow
        
        $HeaderContent += "extern const unsigned char ${Name}_data[];`n"
        $HeaderContent += "extern const size_t ${Name}_size;`n`n"
        
        $SourceContent += "const unsigned char ${Name}_data[] = { 0 };`n"
        $SourceContent += "const size_t ${Name}_size = 0;`n`n"
    }
}

$HeaderContent += @"

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
"@

$SourceContent += @"

#endif
"@

[System.IO.File]::WriteAllText($OutputHeader, $HeaderContent)
[System.IO.File]::WriteAllText($OutputSource, $SourceContent)

Remove-Item -Path $TempDir -Recurse -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "[DONE] Files generated:" -ForegroundColor Green
Write-Host "  - $OutputHeader" -ForegroundColor White
Write-Host "  - $OutputSource" -ForegroundColor White

$TotalSizeKB = [math]::Round($TotalSize / 1024, 2)
$TotalSizeMB = [math]::Round($TotalSize / 1024 / 1024, 2)
Write-Host ""
Write-Host "Total assets size: $TotalSizeKB KB ($TotalSizeMB MB)" -ForegroundColor Cyan
Write-Host ""
