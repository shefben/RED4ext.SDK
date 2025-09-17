# CP2077 Coop Mod - Minimal Fixes for Basic Testing

## Critical Path Analysis: Game Load → Server Connection

This document focuses ONLY on the essential fixes needed to get the mod loading in-game and establishing basic client-server connections for initial testing.

---

## **PHASE 1: COMPILATION FIXES (Must Fix to Build)**

### **FIX #1: Create Missing Logger.hpp**
**CRITICAL - BLOCKS COMPILATION**
- **Files Affected**: `src/server/DedicatedServer.cpp`, `src/runtime/InventoryController.cpp`
- **Issue**: Code calls `LogError()`, `LogInfo()`, `LogWarning()` but no logger exists
- **Quick Fix**: Create basic logger that prints to stdout

```cpp
// Create: cp2077-coop/src/core/Logger.hpp
#pragma once
#include <iostream>
#define LogInfo(fmt, ...) std::cout << "[INFO] " << fmt << std::endl
#define LogError(fmt, ...) std::cerr << "[ERROR] " << fmt << std::endl  
#define LogWarning(fmt, ...) std::cout << "[WARN] " << fmt << std::endl
```

### **FIX #2: Remove Invalid .reds Includes**
**CRITICAL - BLOCKS COMPILATION**
- **Files Affected**: `src/net/Connection.cpp`, `src/net/Net.cpp`, `src/net/SnapshotWriter.cpp`
- **Issue**: C++ files cannot include REDscript files
- **Quick Fix**: Remove these lines:
```cpp
// REMOVE these lines from C++ files:
#include "../runtime/GameModeManager.reds"
#include "../runtime/TileGameSync.reds"
#include "../runtime/QuestSync.reds"
#include "../runtime/SpectatorCam.reds"
```

### **FIX #3: Fix Server Function Name Mismatch**
**CRITICAL - SERVER WON'T START**
- **File**: `src/server/DedicatedServer.cpp:51`
- **Issue**: Calls `Net_Initialize()` but function is actually `Net_Init()`
- **Quick Fix**: Change `Net_Initialize()` to `Net_Init()`

### **FIX #4: Create Missing Server Functions**
**CRITICAL - SERVER WON'T START**
- **File**: `src/server/DedicatedServer.cpp:57-66`
- **Issue**: Calls undefined functions
- **Quick Fix**: Add stub functions to `src/net/Net.cpp`:

```cpp
// Add to Net.cpp:
bool Net_StartServer(uint32_t port, uint32_t maxPlayers) {
    std::cout << "Starting server on port " << port << " for " << maxPlayers << " players" << std::endl;
    // TODO: Implement actual server start logic
    return true;
}

void InitializeGameSystems() {
    std::cout << "Initializing game systems..." << std::endl;
    // TODO: Implement game system initialization
}

void LoadServerPlugins() {
    std::cout << "Loading server plugins..." << std::endl;
    // TODO: Implement plugin loading
}
```

---

## **PHASE 2: NETWORKING INITIALIZATION (Must Fix for Connections)**

### **FIX #5: Add Client-Side Network Initialization**
**HIGH PRIORITY - NO CLIENT CONNECTIONS**
- **Issue**: `Net_Init()` is never called from the game client
- **Quick Fix**: Add network init to main entry point

```cpp
// In src/core/CoopExports.cpp, in Main() function Load case:
case RED4ext::EMainReason::Load:
{
    auto rtti = RED4ext::CRTTISystem::Get();
    rtti->AddRegisterCallback(RegisterTypes);
    rtti->AddPostRegisterCallback(PostRegisterTypes);
    
    // ADD THIS LINE:
    Net_Init();
    
    break;
}

// And in Unload case:
case RED4ext::EMainReason::Unload:
    // ADD THIS LINE:
    Net_Shutdown();
    break;
```

### **FIX #6: Register Network Init Function for REDscript**
**HIGH PRIORITY - REDSCRIPT CAN'T INIT NETWORKING**
- **File**: `src/core/CoopExports.cpp`
- **Issue**: REDscript has no way to initialize networking
- **Quick Fix**: Add native function registration in `PostRegisterTypes()`:

```cpp
// Add after line 451 in PostRegisterTypes():
auto netInit = RED4ext::CGlobalFunction::Create("Net_Initialize", "Net_Initialize", &NetInitFn);
netInit->flags = flags;
netInit->SetReturnType("Bool");
rtti->RegisterFunction(netInit);
```

And add the corresponding function:
```cpp
static void NetInitFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, void*)
{
    aFrame->code++;
    Net_Init();
    if (aOut)
        *aOut = true;
}
```

### **FIX #7: Fix REDscript Connection Functions**
**MEDIUM PRIORITY - UI WON'T CONNECT**
- **File**: `src/runtime/InventorySync.reds:48,57,62,68`
- **Issue**: Calls undefined helper functions
- **Quick Fix**: Add stub implementations or replace with existing functions:

```cpp
// Replace undefined function calls with stubs:
let peerId = 0u; // Instead of GetPlayerPeerId(player)
// Comment out: Net_SendInventorySnapshot(inventorySnap);  
let money = 0u; // Instead of GetPlayerMoney(player)
let version = 1u; // Instead of GetPlayerInventoryVersion(player)
```

---

## **PHASE 3: UI INTEGRATION (For Menu Access)**

### **FIX #8: Make Codeware Dependency Optional** 
**MEDIUM PRIORITY - UI WON'T LOAD**
- **File**: `src/gui/MainMenuInjection.reds:1`
- **Issue**: External dependency not in project
- **Quick Fix**: Wrap in conditional:

```swift
// Replace:
import Codeware.UI

// With:
// import Codeware.UI  // Optional dependency - comment out if not available
```

### **FIX #9: Add Network Init Call to Main Entry**
**MEDIUM PRIORITY - SYSTEMS NOT INITIALIZED**
- **File**: `src/Main.reds:4-6`
- **Issue**: No networking initialization from REDscript side
- **Quick Fix**: Add network init:

```swift
public func OnAttach() -> Void {
    LogChannel(n"DEBUG", "cp2077-coop loaded");
    
    // ADD THIS LINE:
    Net_Initialize();
    
    MainMenuInjection.Inject();
}
```

---

## **BASIC CONNECTION TEST PROCEDURE**

After applying these minimal fixes:

### **1. Build Test:**
```bash
cd cp2077-coop
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### **2. Server Test:**
```bash
./coop_dedicated.exe
# Should print:
# "Starting server on port 7777 for 32 players"
# "Initializing game systems..."
# "Loading server plugins..."
# "Net_Init complete"
```

### **3. Client Test:**
1. Copy `cp2077-coop.dll` to Cyberpunk RED4ext plugins folder
2. Launch Cyberpunk 2077
3. Check logs for: `"[INFO] cp2077-coop loaded"` and `"Net_Init complete"`
4. Main menu should show cooperative button

### **4. Connection Test:**
1. Start dedicated server
2. In game, click cooperative button
3. Try to connect to localhost:7777
4. Should establish basic ENet connection (even if no game sync yet)

---

## **EXPECTED RESULTS AFTER FIXES**

### **✅ What Should Work:**
- Mod compiles successfully
- RED4ext plugin loads in game
- Dedicated server starts without crashing
- Basic ENet networking initializes
- Main menu shows coop button
- Client can attempt connection to server

### **❌ What Won't Work Yet (Next Phase):**
- Actual game synchronization
- Player spawning/despawning  
- Inventory synchronization
- World state synchronization
- Anti-cheat systems
- Save game integration

---

## **FILES TO CREATE/MODIFY**

### **New Files (Create These):**
1. `src/core/Logger.hpp` - Basic logging system

### **Modified Files (Make These Changes):**
1. `src/server/DedicatedServer.cpp` - Fix function names and add logging
2. `src/net/Connection.cpp` - Remove .reds includes
3. `src/net/Net.cpp` - Remove .reds includes, add server functions
4. `src/net/SnapshotWriter.cpp` - Remove .reds includes
5. `src/core/CoopExports.cpp` - Add network init, register functions
6. `src/runtime/InventorySync.reds` - Fix undefined function calls
7. `src/gui/MainMenuInjection.reds` - Make Codeware optional
8. `src/Main.reds` - Add network initialization

---

## **ESTIMATED TIME TO IMPLEMENT**

- **Phase 1 Compilation Fixes**: 30-60 minutes
- **Phase 2 Network Init**: 60-90 minutes  
- **Phase 3 UI Integration**: 30-45 minutes
- **Testing & Validation**: 30-60 minutes

**Total: ~3-5 hours for basic loading and connection capability**

---

This represents the absolute minimum changes needed to get the mod into a testable state where it can load in-game and establish basic client-server connections. All other features can be implemented iteratively after this foundation is working.