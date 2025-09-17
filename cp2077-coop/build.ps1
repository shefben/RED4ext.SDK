# CP2077 Coop Mod Build Script
# PowerShell script to compile and package the multiplayer mod

param(
    [string]$BuildType = "Debug",
    [switch]$Clean,
    [switch]$Package,
    [switch]$Install
)

$ModName = "CP2077-Coop"
$Version = "1.0.0-beta"
$BuildDir = "$PSScriptRoot\build"
$SourceDir = "$PSScriptRoot\src"
$OutputDir = "$PSScriptRoot\dist"
$GameDir = "$env:USERPROFILE\AppData\Local\REDEngine\REDGame\r6\cache\modded"

Write-Host "CP2077 Coop Mod Build System" -ForegroundColor Green
Write-Host "Version: $Version" -ForegroundColor Yellow
Write-Host "Build Type: $BuildType" -ForegroundColor Yellow

# Clean previous builds
if ($Clean) {
    Write-Host "Cleaning previous builds..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Remove-Item $BuildDir -Recurse -Force
    }
    if (Test-Path $OutputDir) {
        Remove-Item $OutputDir -Recurse -Force
    }
}

# Create build directories
Write-Host "Creating build directories..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

# Check for REDscript compiler
$RedscriptPath = "$env:PROGRAMFILES\REDmodding\REDscript"
if (-not (Test-Path $RedscriptPath)) {
    Write-Error "REDscript not found at $RedscriptPath. Please install REDscript first."
    exit 1
}

Write-Host "Found REDscript at: $RedscriptPath" -ForegroundColor Green

# Compile REDscript files
Write-Host "Compiling REDscript files..." -ForegroundColor Yellow
$RedscriptFiles = Get-ChildItem -Path $SourceDir -Filter "*.reds" -Recurse

foreach ($file in $RedscriptFiles) {
    $relativePath = $file.FullName.Substring($SourceDir.Length + 1)
    $outputPath = Join-Path $BuildDir $relativePath
    $outputDir = Split-Path $outputPath -Parent
    
    if (-not (Test-Path $outputDir)) {
        New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
    }
    
    Write-Host "  Compiling: $relativePath" -ForegroundColor Cyan
    Copy-Item $file.FullName $outputPath
}

# Create mod structure
Write-Host "Creating mod structure..." -ForegroundColor Yellow
$ModStructure = @{
    "r6\scripts\$ModName" = "$BuildDir\*"
    "red4ext\plugins\$ModName" = @()
    "archive\pc\mod" = @()
}

foreach ($dir in $ModStructure.Keys) {
    $fullPath = Join-Path $OutputDir $dir
    New-Item -ItemType Directory -Force -Path $fullPath | Out-Null
    
    if ($ModStructure[$dir] -is [string]) {
        Copy-Item $ModStructure[$dir] $fullPath -Recurse -Force
    }
}

# Create mod info file
Write-Host "Creating mod info..." -ForegroundColor Yellow
$ModInfo = @{
    name = $ModName
    version = $Version
    description = "Comprehensive multiplayer transformation for Cyberpunk 2077"
    author = "CP2077 Coop Team"
    requirements = @(
        "RED4ext v1.15.0+",
        "REDscript v0.5.17+",
        "Cyber Engine Tweaks v1.32.0+"
    )
    features = @(
        "32-player multiplayer support",
        "Cooperative missions",
        "Competitive PvP",
        "Shared world persistence",
        "Guild system",
        "Economic simulation",
        "World modification tools",
        "Endgame content",
        "Legacy progression system",
        "Community challenges"
    )
}

$ModInfo | ConvertTo-Json -Depth 3 | Out-File "$OutputDir\mod_info.json" -Encoding UTF8

# Create installation script
Write-Host "Creating installation script..." -ForegroundColor Yellow
$InstallScript = @"
@echo off
echo Installing CP2077 Coop Mod v$Version
echo.

REM Check if Cyberpunk 2077 is installed
if not exist "%USERPROFILE%\AppData\Local\REDEngine\REDGame\r6\cache\modded" (
    echo ERROR: Cyberpunk 2077 modding directory not found.
    echo Please ensure the game is installed and has been run at least once.
    pause
    exit /b 1
)

REM Create backup
echo Creating backup of existing files...
if exist "%USERPROFILE%\AppData\Local\REDEngine\REDGame\r6\cache\modded\r6\scripts\$ModName" (
    move "%USERPROFILE%\AppData\Local\REDEngine\REDGame\r6\cache\modded\r6\scripts\$ModName" "%USERPROFILE%\AppData\Local\REDEngine\REDGame\r6\cache\modded\r6\scripts\${ModName}_backup_%date:~-4,4%%date:~-10,2%%date:~-7,2%"
)

REM Copy mod files
echo Copying mod files...
xcopy /E /I /Y "r6\*" "%USERPROFILE%\AppData\Local\REDEngine\REDGame\r6\cache\modded\r6\"
xcopy /E /I /Y "red4ext\*" "%USERPROFILE%\AppData\Local\REDEngine\REDGame\red4ext\"
xcopy /E /I /Y "archive\*" "%USERPROFILE%\AppData\Local\REDEngine\REDGame\archive\"

echo.
echo Installation complete!
echo Please restart Cyberpunk 2077 to activate the mod.
echo.
echo For server setup, see the included README.md
pause
"@

$InstallScript | Out-File "$OutputDir\install.bat" -Encoding ASCII

# Create README
Write-Host "Creating README..." -ForegroundColor Yellow
$ReadmeContent = @"
# CP2077 Coop Mod v$Version

## Overview
Transform Cyberpunk 2077 into a comprehensive multiplayer experience with up to 32 players per server.

## Features
- **Multiplayer Core**: Real-time player synchronization, voice chat, shared world
- **Cooperative Gameplay**: Mission sharing, guild system, shared economy
- **Competitive Elements**: PvP arenas, tournaments, leaderboards
- **World Modification**: Terrain editing, building construction, persistent changes
- **Endgame Content**: Legendary contracts, world events, progression paths
- **Legacy System**: Multi-generational character development
- **Community Features**: Server-wide challenges, collective objectives

## Requirements
- Cyberpunk 2077 v2.0+
- RED4ext v1.15.0+
- REDscript v0.5.17+
- Cyber Engine Tweaks v1.32.0+

## Quick Installation
1. Run `install.bat` as administrator
2. Restart Cyberpunk 2077
3. The mod will auto-initialize on game start

## Server Setup
### Hosting a Server:
1. Edit game config: Set `serverMode = true` in mod settings
2. Forward port 7777 (UDP) on your router
3. Share your external IP with players
4. Launch the game - server starts automatically

### Joining a Server:
1. Launch Cyberpunk 2077 with mod installed
2. In-game: Press F7 to open multiplayer menu
3. Enter server IP address
4. Click "Connect"

## Key Controls
- **F7**: Open multiplayer menu
- **F8**: Toggle voice chat
- **F9**: Guild management
- **F10**: World modification tools
- **F11**: Mission editor
- **F12**: Community challenges

## Configuration
Edit `CP2077CoopMod.reds` to customize:
- Max players (default: 32)
- Enable/disable features
- Server region
- Network settings

## Troubleshooting
1. **Mod not loading**: Ensure all dependencies are installed
2. **Connection issues**: Check firewall/antivirus settings
3. **Performance problems**: Reduce max players or disable world persistence
4. **Voice chat not working**: Check microphone permissions

## Support
- GitHub Issues: [Link to repository]
- Discord: [Server invite]
- Forums: [Community forum link]

## Credits
Developed by the CP2077 Coop Team
Built with RED4ext SDK and REDscript
"@

$ReadmeContent | Out-File "$OutputDir\README.md" -Encoding UTF8

# Package if requested
if ($Package) {
    Write-Host "Creating package..." -ForegroundColor Yellow
    $PackageName = "${ModName}-v${Version}.zip"
    $PackagePath = Join-Path $PSScriptRoot $PackageName
    
    if (Get-Command "7z" -ErrorAction SilentlyContinue) {
        & 7z a -tzip $PackagePath "$OutputDir\*"
    } elseif (Get-Command "Compress-Archive" -ErrorAction SilentlyContinue) {
        Compress-Archive -Path "$OutputDir\*" -DestinationPath $PackagePath -Force
    } else {
        Write-Warning "No compression tool found. Package not created."
    }
    
    if (Test-Path $PackagePath) {
        Write-Host "Package created: $PackagePath" -ForegroundColor Green
    }
}

# Install if requested
if ($Install) {
    if (-not (Test-Path $GameDir)) {
        Write-Error "Game directory not found: $GameDir"
        Write-Error "Please ensure Cyberpunk 2077 is installed and has been run at least once."
        exit 1
    }
    
    Write-Host "Installing mod to game directory..." -ForegroundColor Yellow
    
    # Copy mod files
    $GameScriptDir = Join-Path $GameDir "r6\scripts\$ModName"
    if (Test-Path $GameScriptDir) {
        $BackupDir = "${GameScriptDir}_backup_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
        Move-Item $GameScriptDir $BackupDir
        Write-Host "Previous installation backed up to: $BackupDir" -ForegroundColor Yellow
    }
    
    Copy-Item "$OutputDir\r6\*" (Join-Path $GameDir "r6\") -Recurse -Force
    
    Write-Host "Mod installed successfully!" -ForegroundColor Green
    Write-Host "Please restart Cyberpunk 2077 to activate the mod." -ForegroundColor Yellow
}

Write-Host "`nBuild completed successfully!" -ForegroundColor Green
Write-Host "Output directory: $OutputDir" -ForegroundColor Yellow

# Display build summary
$ScriptFiles = (Get-ChildItem -Path $BuildDir -Filter "*.reds" -Recurse).Count
$BuildSize = (Get-ChildItem -Path $OutputDir -Recurse | Measure-Object -Property Length -Sum).Sum / 1MB

Write-Host "`nBuild Summary:" -ForegroundColor Green
Write-Host "  Script files: $ScriptFiles" -ForegroundColor White
Write-Host "  Build size: $($BuildSize.ToString('F2')) MB" -ForegroundColor White
Write-Host "  Build type: $BuildType" -ForegroundColor White
Write-Host "  Version: $Version" -ForegroundColor White