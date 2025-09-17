# CP2077 Coop Mod - Complete Installation & Usage Guide

## Table of Contents
1. [System Requirements](#system-requirements)
2. [Prerequisites Installation](#prerequisites-installation)
3. [Mod Compilation](#mod-compilation)
4. [Mod Installation](#mod-installation)
5. [Server Setup](#server-setup)
6. [Client Setup](#client-setup)
7. [Feature Overview](#feature-overview)
8. [Troubleshooting](#troubleshooting)
9. [Configuration](#configuration)

## System Requirements

### Minimum Requirements
- **Game**: Cyberpunk 2077 v2.0 or later
- **OS**: Windows 10/11 (64-bit) or Linux (Steam Proton)
- **CPU**: Intel i5-8400 / AMD Ryzen 5 2600
- **RAM**: 16 GB (32 GB recommended for servers)
- **Storage**: 5 GB free space for mod files
- **Network**: Broadband connection (server hosting requires port forwarding)

### Recommended for Server Hosting
- **CPU**: Intel i7-10700K / AMD Ryzen 7 3700X or better
- **RAM**: 32 GB or more
- **Network**: Upload speed 10+ Mbps for 32 players
- **Storage**: SSD recommended

## Prerequisites Installation

Before installing the mod, you need these essential tools:

### 1. RED4ext Framework
RED4ext provides the core modding framework for Cyberpunk 2077.

**Installation:**
1. Download RED4ext v1.15.0+ from [GitHub releases](https://github.com/WopsS/RED4ext/releases)
2. Extract the ZIP file
3. Copy `red4ext` folder to your Cyberpunk 2077 root directory
4. Copy `version.dll` to your Cyberpunk 2077 root directory

**Verify Installation:**
- Launch Cyberpunk 2077
- Check logs in `Cyberpunk 2077/red4ext/logs/` for successful loading

### 2. REDscript Compiler
REDscript enables script compilation and execution.

**Installation:**
1. Download REDscript v0.5.17+ from [GitHub releases](https://github.com/jac3km4/redscript/releases)
2. Extract to `C:\Program Files\REDmodding\REDscript\` (Windows) or `/opt/redscript/` (Linux)
3. Add the installation directory to your PATH environment variable

**Verify Installation:**
```bash
# Windows
redscript --version

# Linux  
/opt/redscript/redscript --version
```

### 3. Cyber Engine Tweaks (Optional but Recommended)
Provides additional modding capabilities and debugging tools.

**Installation:**
1. Download CET v1.32.0+ from [GitHub releases](https://github.com/yamashi/CyberEngineTweaks/releases)
2. Extract to your Cyberpunk 2077 root directory
3. Launch the game to complete installation

## Mod Compilation

### Windows Compilation

1. **Open PowerShell as Administrator**
2. **Navigate to mod directory:**
   ```powershell
   cd "F:\development\steam\emulator_bot\RED4ext.SDK.codex\cp2077-coop"
   ```

3. **Clean build (optional):**
   ```powershell
   .\build.ps1 -Clean
   ```

4. **Compile mod:**
   ```powershell
   # Debug build (recommended for testing)
   .\build.ps1 -BuildType Debug
   
   # Release build (for distribution)
   .\build.ps1 -BuildType Release
   ```

5. **Create package:**
   ```powershell
   .\build.ps1 -Package
   ```

6. **Direct install:**
   ```powershell
   .\build.ps1 -Install
   ```

### Linux/macOS Compilation

1. **Open Terminal**
2. **Navigate to mod directory:**
   ```bash
   cd /path/to/cp2077-coop
   ```

3. **Set game directory:**
   ```bash
   export CYBERPUNK_DIR="$HOME/.steam/steam/steamapps/common/Cyberpunk 2077"
   ```

4. **Build mod:**
   ```bash
   make build
   ```

5. **Create package:**
   ```bash
   make package
   ```

6. **Install directly:**
   ```bash
   make install
   ```

## Mod Installation

### Automatic Installation (Windows)

1. **Run the installer:**
   ```
   dist\install.bat
   ```
   - Run as Administrator
   - The script will automatically detect your game directory
   - Creates backup of existing files

### Manual Installation

1. **Locate your Cyberpunk 2077 directory:**
   ```
   # Steam (Windows)
   C:\Program Files (x86)\Steam\steamapps\common\Cyberpunk 2077\
   
   # GOG (Windows)
   C:\GOG Games\Cyberpunk 2077\
   
   # Epic Games (Windows)  
   C:\Program Files\Epic Games\Cyberpunk2077\
   
   # Steam (Linux)
   ~/.steam/steam/steamapps/common/Cyberpunk 2077/
   ```

2. **Copy mod files:**
   ```
   # Copy REDscript files
   cp -r dist/r6/* "Cyberpunk 2077/r6/"
   
   # Copy RED4ext plugins (if any)
   cp -r dist/red4ext/* "Cyberpunk 2077/red4ext/"
   
   # Copy archive mods (if any)
   cp -r dist/archive/* "Cyberpunk 2077/archive/"
   ```

3. **Verify installation:**
   - Check that files exist in: `Cyberpunk 2077/r6/scripts/CP2077-Coop/`
   - Launch game and check logs for mod loading

## Server Setup

### Setting Up a Dedicated Server

1. **Configure server mode:**
   Edit `CP2077CoopMod.reds` in your scripts directory:
   ```typescript
   defaultConfig.serverMode = true;
   defaultConfig.maxPlayers = 32;  // Adjust based on your hardware
   defaultConfig.networkPort = 7777;
   ```

2. **Network configuration:**
   - **Port Forward**: Open port 7777 (UDP) on your router
   - **Firewall**: Allow Cyberpunk 2077 through Windows Firewall
   - **Find your external IP**: Visit whatismyipaddress.com

3. **Launch server:**
   ```bash
   # Start Cyberpunk 2077 normally
   # The mod will auto-detect server mode and start the server
   ```

4. **Verify server status:**
   - Check game console (F1) for "Server started successfully" message
   - Monitor `red4ext/logs/` for connection attempts
   - Server info displayed in game UI (F7)

### Server Administration

**In-game commands (F1 console):**
```
# Player management
!kick <playerId> <reason>
!ban <playerId> <duration> <reason>
!unban <playerId>

# Server control  
!restart <seconds>
!shutdown
!save_world
!load_world <saveName>

# Mod management
!reload_config
!enable_feature <featureName>
!disable_feature <featureName>
```

**Configuration options:**
```typescript
// In CP2077CoopMod.reds
defaultConfig.maxPlayers = 32;          // 4-64 players
defaultConfig.enableVoiceChat = true;   // Voice communication
defaultConfig.enablePvP = true;         // Player vs Player
defaultConfig.enableWorldPersistence = true; // Save world changes
defaultConfig.enableSharedEconomy = true;    // Shared money/items
defaultConfig.enableGuilds = true;      // Guild system
defaultConfig.enableEndgameContent = true;   // High-level content
```

## Client Setup

### Joining a Server

1. **Launch Cyberpunk 2077** with mod installed
2. **Open multiplayer menu** (F7 key)
3. **Enter server details:**
   - Server IP: `xxx.xxx.xxx.xxx:7777`
   - Password: (if required)
   - Player name: Your display name
4. **Click "Connect"**

### Client Configuration

**Performance settings:**
```typescript
// For lower-end systems
defaultConfig.maxPlayers = 16;  // Reduce if experiencing lag
defaultConfig.enableWorldPersistence = false;  // Disable for performance
defaultConfig.debugMode = false;  // Disable debug output
```

**Network settings:**
```typescript
defaultConfig.networkPort = 7777;  // Must match server
defaultConfig.serverRegion = "US";  // Auto-detect or specify
```

## Feature Overview

### Core Multiplayer Features

#### 1. Real-time Player Synchronization
- **Position sync**: All player movements replicated in real-time
- **Action sync**: Combat, interactions, and animations synchronized
- **State sync**: Health, cyberware, and status effects shared
- **Vehicle sync**: All vehicles and their occupants synchronized

**Usage:**
- Simply move around - other players see you automatically
- Combat actions affect other players in PvP areas
- Use vehicles normally - passengers synchronized automatically

#### 2. Voice Communication System
- **Proximity voice**: Talk to nearby players (3D spatial audio)
- **Party voice**: Private channel for your group
- **Guild voice**: Guild-wide communication channel
- **Global voice**: Server-wide announcements (admin only)

**Controls:**
- **F8**: Toggle voice chat on/off
- **V (hold)**: Push-to-talk (proximity)
- **B (hold)**: Push-to-talk (party)
- **G (hold)**: Push-to-talk (guild)

#### 3. Shared World Persistence
- **World changes**: Modifications persist across sessions
- **Item placement**: Objects remain where players place them
- **Building persistence**: Constructed buildings saved permanently
- **Economy state**: Market prices and availability synchronized

### Cooperative Features

#### 4. Mission Sharing System
- **Shared objectives**: All party members get same objectives
- **Progress sync**: Objective progress shared across party
- **Reward distribution**: Configurable reward splitting
- **Difficulty scaling**: Missions scale with party size

**Usage:**
1. Form a party (F7 menu → Parties → Create/Join)
2. Start any mission - party members automatically join
3. Complete objectives together
4. Rewards distributed based on contribution

#### 5. Guild System
- **Guild creation**: Form persistent organizations
- **Rank system**: Hierarchical member management
- **Guild halls**: Shared spaces for guild activities
- **Guild missions**: Exclusive multi-member objectives

**Guild Management:**
- **F9**: Open guild management interface
- **Create guild**: Costs 50,000 eddies
- **Invite players**: `/guild invite <playerName>`
- **Promote members**: Guild leader privileges
- **Guild bank**: Shared resource storage

#### 6. Shared Economy
- **Global marketplace**: Server-wide trading system
- **Dynamic pricing**: Supply/demand affects prices
- **Player shops**: Set up your own stores
- **Auction house**: Bid on rare items

**Trading:**
1. Open marketplace (F7 → Economy → Marketplace)
2. Browse items or search specific categories
3. Purchase instantly or place bids
4. List your own items for sale
5. Manage your shop (if established)

### Competitive Features

#### 7. PvP Arena System
- **Dedicated arenas**: Balanced combat zones
- **Ranked matches**: ELO-based competitive system
- **Tournament mode**: Organized competitions
- **Spectator mode**: Watch matches in progress

**PvP Zones:**
- **Watson Combat Arena**: Close-quarters combat
- **Badlands Raceway**: Vehicle-based combat
- **Corporate Plaza**: Urban warfare simulation
- **Afterlife Underground**: No-holds-barred fighting

#### 8. Competitive Leaderboards
- **Combat rankings**: PvP statistics and ratings
- **Wealth leaderboards**: Richest players on server
- **Achievement rankings**: Most accomplished players
- **Guild rankings**: Most powerful organizations

### World Modification Features

#### 9. Terrain Editing Tools
- **Landscape modification**: Reshape terrain in designated areas
- **Building construction**: Place and construct buildings
- **Infrastructure**: Roads, bridges, and utilities
- **Decoration**: Furniture, lighting, and aesthetics

**Controls:**
- **F10**: Open world modification interface
- **Build Mode**: Select construction/modification tools
- **Resource management**: Track required materials
- **Permission system**: Area ownership and building rights

#### 10. Character Creation Plus
- **Extended customization**: Additional appearance options
- **Shared presets**: Share character designs with others
- **Import/Export**: Save character templates
- **Community gallery**: Browse public character designs

### Endgame Content

#### 11. Legendary Contracts
- **Ultra-difficult missions**: Requiring maximum coordination
- **10 difficulty tiers**: From Legendary to "Beyond Comprehension"
- **World-shaping rewards**: Permanent server modifications
- **Prerequisites**: High-level characters and specific equipment

**Difficulty Tiers:**
1. **Legendary**: 4-6 players, high-end gear required
2. **Mythic**: 8-12 players, perfect coordination needed
3. **Transcendent**: 16+ players, server-wide preparation
4. **Cosmic**: 24+ players, months of preparation
5. **Universal**: Full server participation required
6. **Multiversal**: Multiple server coordination
7. **Reality-Breaking**: Unknown requirements
8. **Existence-Defying**: Theoretical difficulty
9. **Comprehension-Transcending**: Beyond understanding
10. **Beyond Comprehension**: Impossible by design

#### 12. World Events System
- **Global crises**: Server-wide emergencies requiring collective response
- **Community building**: Collaborative construction projects
- **Apocalyptic scenarios**: High-stakes survival challenges
- **Permanent consequences**: Events that permanently alter the world

**Event Types:**
- **AI Uprising**: Combat scenario across all districts
- **Corporate War**: Economic and military conflict
- **Dimensional Breach**: Reality-altering phenomenon
- **Resource Crisis**: Scarcity requiring cooperation
- **Cultural Revolution**: Social transformation event

#### 13. Legacy System
- **Multi-generational progression**: Character advancement beyond death
- **Essence collection**: Spiritual/digital advancement currency
- **Transcendent abilities**: Powers beyond normal human limits
- **Digital immortality**: Consciousness preservation and transfer
- **Legacy artifacts**: Inheritable legendary items

**Progression Path:**
1. **Mortal**: Standard character advancement
2. **Enhanced**: Cybernetic transcendence
3. **Digital Backup**: Consciousness copying
4. **Consciousness Transfer**: Body-independent existence
5. **Reality Anchor**: Ability to affect world permanently
6. **Temporal Loop**: Limited time manipulation
7. **Dimensional Existence**: Multi-dimensional presence
8. **Cosmic Entity**: Universe-scale influence
9. **Universal Constant**: Fundamental reality component
10. **Transcendent Being**: Beyond comprehension

#### 14. Community Challenges
- **Collective objectives**: Goals requiring entire server participation
- **Resource mobilization**: Massive construction/research projects
- **Crisis response**: Emergency scenarios needing coordination
- **Cultural achievements**: Server-wide social accomplishments

## Troubleshooting

### Common Installation Issues

**Problem: Mod doesn't load**
- **Solution**: Check RED4ext installation and logs
- **Verify**: `red4ext/logs/` contains loading messages
- **Fix**: Reinstall RED4ext framework

**Problem: Compilation errors**
- **Solution**: Verify REDscript installation
- **Check**: REDscript is in PATH environment variable
- **Fix**: Update to latest REDscript version

**Problem: Game crashes on startup**
- **Solution**: Check mod compatibility
- **Verify**: Remove other conflicting mods
- **Fix**: Use clean game installation for testing

### Networking Issues

**Problem: Cannot connect to server**
- **Check**: Server IP and port are correct
- **Verify**: Firewall allows Cyberpunk 2077
- **Fix**: Try different port or disable VPN

**Problem: High latency/lag**
- **Check**: Network connection stability
- **Reduce**: Number of connected players
- **Fix**: Enable performance optimizations

**Problem: Voice chat not working**
- **Check**: Microphone permissions
- **Verify**: Audio device is set correctly
- **Fix**: Restart game and check audio settings

### Performance Issues

**Problem: Low FPS in multiplayer**
- **Reduce**: `maxPlayers` setting
- **Disable**: `enableWorldPersistence`
- **Lower**: Graphics settings in game
- **Close**: Unnecessary background applications

**Problem: Memory usage high**
- **Increase**: Virtual memory/swap file
- **Reduce**: Number of active features
- **Restart**: Game periodically to clear memory

## Configuration

### Advanced Configuration Options

Edit `src/CP2077CoopMod.reds` for advanced settings:

```typescript
public struct ModConfiguration {
    // Core multiplayer settings
    public let maxPlayers: Int32 = 32;          // 4-64 players supported
    public let networkPort: Int32 = 7777;       // UDP port for networking
    public let serverRegion: String = "AUTO";   // AUTO, US, EU, ASIA
    
    // Feature toggles
    public let enableVoiceChat: Bool = true;
    public let enablePvP: Bool = true;
    public let enableWorldPersistence: Bool = true;
    public let enableSharedEconomy: Bool = true;
    public let enableGuilds: Bool = true;
    public let enableWorldModification: Bool = true;
    public let enableEndgameContent: Bool = true;
    public let enableLegacySystem: Bool = true;
    public let enableCommunityChallenges: Bool = true;
    
    // Performance settings
    public let debugMode: Bool = false;          // Enable debug logging
    public let maxSyncDistance: Float = 1000.0; // Player sync range
    public let updateFrequency: Int32 = 60;      // Network updates per second
    public let compressionLevel: Int32 = 6;      // Network compression (1-9)
    
    // Server-specific settings
    public let serverMode: Bool = false;         // Enable server hosting
    public let adminPassword: String = "";       // Admin access password
    public let serverName: String = "CP2077 Coop Server";
    public let serverDescription: String = "";
    public let serverTags: array<String> = [];   // Server browser tags
}
```

### Performance Tuning

**For low-end systems:**
```typescript
maxPlayers = 8;
enableWorldPersistence = false;
maxSyncDistance = 500.0;
updateFrequency = 30;
compressionLevel = 9;
```

**For high-end servers:**
```typescript
maxPlayers = 64;
enableWorldPersistence = true;
maxSyncDistance = 2000.0;
updateFrequency = 120;
compressionLevel = 1;
```

### Mod Development

**Creating custom content:**
1. Study existing `.reds` files in `src/` directory
2. Follow REDscript syntax and conventions
3. Test changes with `debugMode = true`
4. Use RED4ext SDK for native extensions
5. Submit contributions via GitHub pull requests

**API Extensions:**
```typescript
// Example: Custom mission type
public class CustomMissionType extends CooperativeMission {
    public func Initialize(missionSpecs: CustomMissionSpecs) -> Bool {
        // Your custom mission logic here
        return true;
    }
}
```

---

## Support and Community

- **GitHub Repository**: [Link to source code and issue tracker]
- **Discord Server**: [Community chat and support]
- **Forums**: [Discussion and mod sharing]
- **Documentation**: [Additional technical documentation]

For additional help or to report bugs, please use the GitHub issue tracker with detailed information about your system and the steps to reproduce any problems.