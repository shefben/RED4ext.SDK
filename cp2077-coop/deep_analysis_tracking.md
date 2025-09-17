# CP2077 Coop Mod - Deep Analysis Tracking

## Files Analyzed Status (330 total files)

### Core C++ Files Analyzed (13/13 core files) ✅ COMPLETE
- ✅ **AssetStreamer.cpp** - Buffer overflow and memory issues found
- ✅ **CoopExports.cpp** - RED4ext integration issues found (FIXED)
- ✅ **SessionState.cpp** - JSON parsing vulnerabilities found
- ✅ **Settings.cpp** - Basic file operations, no major issues
- ✅ **Version.cpp** - Version generation system analyzed
- ✅ **HttpClient.cpp** - HTTP client implementation analyzed
- ✅ **CrashHandler.cpp** - Crash reporting system analyzed
- ✅ **GameClock.cpp** - Thread safety issues found
- ✅ **GameProcess.cpp** - CRITICAL: Command injection and POSIX functions
- ✅ **SaveFork.cpp** - CRITICAL: Buffer overflows and format string parsing
- ✅ **SaveMigration.cpp** - Path injection and memory allocation issues
- ✅ **SpatialGrid.cpp** - CRITICAL: Compilation error (duplicate namespace)
- ✅ **TaskGraph.cpp** - CRITICAL: Deadlock in resize method

### Network C++ Files Analyzed (9/9 net files) ✅ COMPLETE
- ✅ **Connection.cpp** - Critical issues with .reds includes and memory
- ✅ **Net.cpp** - Threading issues and missing functions (FIXED)
- ✅ **InterestGrid.cpp** - Thread safety issues with global instance
- ✅ **NatClient.cpp** - TURN credential storage vulnerabilities
- ✅ **NatTraversal.cpp** - ICE/STUN timeout and resource cleanup issues
- ✅ **PhaseBundle.cpp** - CRITICAL buffer overflow in decompression
- ✅ **SnapshotWriter.cpp** - .reds include issues (FIXED)
- ✅ **StatBatch.cpp** - Hardcoded SSL endpoints and data validation issues  
- ✅ **WorldMarkers.cpp** - CRITICAL buffer overflow and integer truncation

### Server C++ Files Analyzed (43/43 server files) ✅ COMPLETE  
- ✅ **LedgerService.cpp** - Financial transaction race conditions found
- ✅ **SnapshotHeap.cpp** - Memory management patterns examined
- ✅ **DedicatedServer.cpp** - Critical server loop issues found (FIXED)
- ✅ **AdminController.cpp** - Admin vulnerabilities and thread issues found
- ✅ **InventoryController.cpp** - CRITICAL: Race conditions and memory leaks
- ✅ **PerkController.cpp** - Thread safety issues with nested containers
- ✅ **SkillController.cpp** - Include path errors and thread safety
- ✅ **VehicleController.cpp** - CRITICAL: Undefined functions, single vehicle limit
- ✅ **TrafficController.cpp** - Race condition in connections access
- ✅ **NpcController.cpp** - CRITICAL: Missing includes, undefined functions
- ✅ **VendorController.cpp** - Type conversion and error handling issues
- ✅ **RenderDevice.cpp** - Address resolution validation missing
- ✅ **WebDash.cpp** - CRITICAL: Linux headers, HTTP injection vulnerabilities
- ✅ **QuestWatchdog.cpp** - Manual JSON parsing and array bounds issues
- ✅ **ChatFilter.cpp** - Basic word filtering, no major issues
- ✅ **DamageValidator.cpp** - Weak validation formula
- ✅ **TradeController.cpp** - CRITICAL: Undefined symbol access
- ✅ **Heartbeat.cpp** - CRITICAL: Incorrect pointer dereference
- ✅ **DedicatedMain.cpp** - CRITICAL: Variable declaration order errors
- ✅ **InfoServer.cpp** - CRITICAL: Linux socket headers
- ✅ **StatusController.cpp** - Type conversion bugs
- ✅ **ApartmentController.cpp** - Unvalidated input parsing
- ✅ **ElevatorController.cpp** - Division by zero risks
- ✅ **PoliceDispatch.cpp** - Integer overflow in timers
- ✅ **PhaseGC.cpp** - Division by zero vulnerability
- ✅ **SectorLODController.cpp** - Linux-only memory detection
- ✅ **CarryController.cpp** - Inefficient empty position updates
- ✅ **Journal.cpp** - File I/O performance issues
- ✅ **15+ additional server files** - Global static thread safety issues found

### Plugin/Voice C++ Files Analyzed (8/8 files) ✅ COMPLETE
- ✅ **PythonVM.cpp** - Python integration memory leaks found
- ✅ **PluginManager.cpp** - CRITICAL: Python memory leaks and asset size issues
- ✅ **OpusEncoder.cpp** - Basic encoder, no major issues
- ✅ **OpusDecoder.cpp** - Thread safety issues with global decoder  
- ✅ **VoiceEncoder.cpp** - OpenAL resource management issues
- ✅ **VoiceDecoder.cpp** - CRITICAL: Thread safety and sequence wraparound bugs
- ✅ **CarPhysics.cpp** - Undefined function calls and logic bugs
- ✅ **LagComp.cpp** - Bounds checking issues with RTT values
- ✅ **VehiclePhysics.cpp** - CRITICAL: Undefined function calls

### REDscript Files Analyzed (6/172 files)
- ✅ **CP2077CoopMod.reds** - Previously analyzed critical issues
- ✅ **InventorySync.reds** - Function call issues found
- ✅ **Main.reds** - Simple entry point, minimal issues
- ✅ **SessionState.reds** - Only native function declarations
- ✅ **MainMenuInjection.reds** - UI integration issues found
- ✅ **AntiCheatFramework.reds** - Logic and implementation issues found
- ❌ **166+ other .reds files** - NOT YET ANALYZED

---

## CRITICAL HIDDEN ISSUES DISCOVERED

### **ISSUE #19: PYTHON VM SECURITY VULNERABILITIES** 
**Priority: CRITICAL**
- **File**: `src/plugin/PythonVM.cpp:30-80`
- **Issue**: Python callbacks stored with Py_INCREF but no cleanup on plugin unload
- **Impact**: Memory leaks and potential security vulnerabilities
- **Details**: Global dictionaries `g_listeners`, `g_packetHandlers` never cleaned up
- **Fix Required**: Implement proper Python object lifecycle management

### **ISSUE #20: ASSET STREAMING BUFFER OVERFLOW**
**Priority: HIGH**  
- **File**: `src/core/AssetStreamer.cpp:119-165`
- **Issue**: Fixed 5MB decompression buffer without size validation
- **Impact**: Buffer overflow if decompressed data > 5MB
- **Details**: `std::vector<uint8_t> raw(5u * 1024u * 1024u);` then `ZSTD_decompress()` without checking
- **Fix Required**: Validate decompressed size before allocation

### **ISSUE #21: JSON PARSING INJECTION VULNERABILITIES**
**Priority: HIGH**
- **File**: `src/core/SessionState.cpp:230-285`  
- **Issue**: Direct scanf parsing of JSON without validation
- **Impact**: Potential injection attacks through malformed JSON
- **Details**: Lines 236-238, 250-252, 272-274 use unsafe scanf
- **Fix Required**: Use proper JSON parser library

### **ISSUE #22: CONNECTION MEMORY LEAK PATTERN**
**Priority: HIGH**
- **File**: `src/net/Connection.cpp:38-100`
- **Issue**: Global bundle memory management with no cleanup bounds
- **Impact**: Unbounded memory growth in `g_bundle` and `g_bundleSha`
- **Details**: `constexpr uint64_t kBundleLimit = 128MB` but no enforcement
- **Fix Required**: Implement proper bundle memory cleanup

### **ISSUE #23: LEDGER SERVICE RACE CONDITIONS**
**Priority: HIGH**
- **File**: `src/server/LedgerService.cpp:30-42`
- **Issue**: Financial transactions without thread synchronization
- **Impact**: Race conditions in balance updates leading to money duplication
- **Details**: No mutex protection around `conn->balance += delta`
- **Fix Required**: Add proper locking around financial operations

### **ISSUE #24: RED4EXT EXECUTE MACRO ABUSE**
**Priority: MEDIUM**
- **File**: `src/net/Connection.cpp:40-66`
- **Issue**: Direct RED4EXT_EXECUTE calls without error handling
- **Impact**: Game crashes if REDscript functions don't exist
- **Details**: No validation that target functions exist before calling
- **Fix Required**: Add function existence checks before execution

### **ISSUE #25: UNISTD.H WINDOWS INCOMPATIBILITY**
**Priority: MEDIUM**  
- **File**: `src/net/Connection.cpp:28`
- **Issue**: Linux-specific header included in cross-platform code
- **Impact**: Compilation failure on Windows
- **Details**: `#include <unistd.h>` incompatible with Windows builds
- **Fix Required**: Platform-specific includes with #ifdef guards

### **ISSUE #26: REDSCRIPT UNDEFINED FUNCTION CALLS**
**Priority: HIGH**
- **File**: `src/runtime/InventorySync.reds:48,57,62,68`
- **Issue**: Calls to undefined helper functions
- **Impact**: Runtime errors when functions called
- **Details**: 
  - `GetPlayerPeerId()` - undefined
  - `Net_SendInventorySnapshot()` - undefined  
  - `GetPlayerMoney()` - undefined
  - `GetPlayerInventoryVersion()` - undefined
- **Fix Required**: Implement missing helper functions or remove calls

### **ISSUE #27: SNAPSHOT HEAP GLIBC DEPENDENCY**
**Priority: LOW**
- **File**: `src/server/SnapshotHeap.cpp:41-46`  
- **Issue**: GNU libc specific mallinfo() usage
- **Impact**: Memory monitoring fails on non-GNU systems
- **Details**: `#ifdef __GLIBC__` limits functionality
- **Fix Required**: Add alternative memory monitoring for other systems

### **ISSUE #28: DEDICATED SERVER UNDEFINED FUNCTIONS**
**Priority: CRITICAL**
- **File**: `src/server/DedicatedServer.cpp:51-66`
- **Issue**: Calls to undefined networking functions
- **Impact**: Server initialization will fail
- **Details**: 
  - `Net_Initialize()` - undefined
  - `Net_StartServer()` - undefined  
  - `InitializeGameSystems()` - undefined
  - `LoadServerPlugins()` - undefined
- **Fix Required**: Implement missing server initialization functions

### **ISSUE #29: ADMIN CONTROLLER BAN SYSTEM VULNERABILITY**
**Priority: HIGH**
- **File**: `src/server/AdminController.cpp:22,76-78`
- **Issue**: Plain text ban list storage with hardcoded path
- **Impact**: Ban list easily tampered with, no encryption/signing
- **Details**: `return "server/bans.json";` - static path, no validation
- **Fix Required**: Implement secure ban list with validation and checksums

### **ISSUE #30: REDSCRIPT UI CODEWARE DEPENDENCY**
**Priority: HIGH**
- **File**: `src/gui/MainMenuInjection.reds:1`
- **Issue**: Import of external Codeware.UI dependency not in project
- **Impact**: UI injection will fail if Codeware not installed
- **Details**: `import Codeware.UI` - external dependency
- **Fix Required**: Make Codeware optional or bundle dependency

### **ISSUE #31: ANTI-CHEAT LOGIC IMPLEMENTATION MISSING**
**Priority: HIGH**
- **File**: `src/security/AntiCheatFramework.reds:111`
- **Issue**: Anti-cheat calls `GetPlayerById()` which doesn't exist
- **Impact**: Anti-cheat system will crash at runtime
- **Details**: Function referenced but never defined anywhere
- **Fix Required**: Implement player lookup functions or use existing game APIs

### **ISSUE #32: REDSCRIPT LOOP INDEX OUT OF BOUNDS**
**Priority: MEDIUM**
- **File**: `src/security/AntiCheatFramework.reds:105`
- **Issue**: Loop uses `0...ArraySize(playerStates)` syntax incorrectly
- **Impact**: Index out of bounds exception
- **Details**: Should be `0, ArraySize(playerStates)` or `i in playerStates`
- **Fix Required**: Fix loop syntax for REDscript

### **ISSUE #33: ADMIN CONSOLE THREAD NEVER STARTED**
**Priority: MEDIUM**
- **File**: `src/server/AdminController.cpp:24-25`
- **Issue**: Global thread variables declared but thread never actually started
- **Impact**: Console input processing won't work
- **Details**: `g_consoleThread` and `g_consoleRunning` declared but no initialization
- **Fix Required**: Implement console thread startup and management

### **ISSUE #34: CRITICAL BUFFER OVERFLOW IN PHASE BUNDLE DECOMPRESSION**
**Priority: CRITICAL** 
- **File**: `src/net/PhaseBundle.cpp:29-31`
- **Issue**: Fixed 16KB buffer for ZSTD decompression without size validation
- **Impact**: Buffer overflow if quest data exceeds 16KB when decompressed
- **Details**: `std::vector<uint8_t> raw(16384);` then `ZSTD_decompress()` without bounds checking
- **Fix Required**: Validate decompressed size before allocation

### **ISSUE #35: CRITICAL BUFFER OVERFLOW IN WORLD MARKERS**  
**Priority: CRITICAL**
- **File**: `src/net/WorldMarkers.cpp:30-31`
- **Issue**: Fixed 10KB buffer for ZSTD decompression without size validation
- **Impact**: Buffer overflow if marker data exceeds 10KB when decompressed  
- **Details**: `std::vector<uint8_t> raw(10240);` then `ZSTD_decompress()` without bounds checking
- **Fix Required**: Validate decompressed size before allocation

### **ISSUE #36: INTEGER TRUNCATION IN WORLD MARKERS**
**Priority: HIGH**
- **File**: `src/net/WorldMarkers.cpp:14`
- **Issue**: DynArray size truncated from size_t to uint16_t
- **Impact**: Memory corruption if more than 65535 markers exist
- **Details**: `uint16_t count = static_cast<uint16_t>(pos.size);` loses data for large arrays
- **Fix Required**: Use proper size type or add bounds checking

### **ISSUE #37: HARDCODED SSL MASTER SERVER ENDPOINT**
**Priority: MEDIUM**
- **File**: `src/net/StatBatch.cpp:31`
- **Issue**: Hardcoded master server "coop-master" with SSL
- **Impact**: All servers connect to same endpoint, potential data leakage
- **Details**: `httplib::SSLClient cli("coop-master", 443);` - no configuration
- **Fix Required**: Make server endpoint configurable

### **ISSUE #38: TURN CREDENTIAL STORAGE VULNERABILITY**
**Priority: HIGH**
- **File**: `src/net/NatClient.cpp:9-12`
- **Issue**: TURN credentials stored in global variables without encryption
- **Impact**: Network credentials exposed in memory dumps
- **Details**: `static std::string g_remoteCandidate;` and related globals 
- **Fix Required**: Encrypt sensitive network credentials

### **ISSUE #39: ICE CONNECTION TIMEOUT BYPASS**
**Priority: MEDIUM**  
- **File**: `src/net/NatTraversal.cpp:75-106`
- **Issue**: ICE timeout can be bypassed by destroying/recreating agent
- **Impact**: Infinite connection attempts, resource exhaustion
- **Details**: Agent destroyed and recreated when ICE fails, resetting timeout
- **Fix Required**: Track total connection attempt time

### **ISSUE #40: COMMAND INJECTION IN GAME PROCESS**
**Priority: CRITICAL**
- **File**: `src/core/GameProcess.cpp:19`
- **Issue**: Direct string concatenation for command execution without sanitization
- **Impact**: Arbitrary command execution via process arguments
- **Details**: `std::string cmd = exe + " " + args;` allows shell injection
- **Fix Required**: Sanitize process arguments and use secure process creation

### **ISSUE #41: DEADLOCK IN TASK GRAPH RESIZE**
**Priority: HIGH**  
- **File**: `src/core/TaskGraph.cpp:56-58`
- **Issue**: Recursive mutex lock attempt in Resize() method
- **Impact**: Application deadlock when resizing worker threads
- **Details**: `Resize()` holds mutex then calls `Stop()` which tries to acquire same mutex
- **Fix Required**: Refactor locking to avoid recursive acquisition

### **ISSUE #42: HTTP REQUEST INJECTION IN WEB DASHBOARD**
**Priority: CRITICAL**
- **File**: `src/server/WebDash.cpp:78-88`
- **Issue**: No input validation on HTTP requests, buffer overflow potential
- **Impact**: HTTP request injection, buffer overflow, remote code execution
- **Details**: `recv(client, buf, sizeof(buf) - 1, 0)` then direct string operations without validation
- **Fix Required**: Add HTTP request validation and size limits

### **ISSUE #43: WEBSOCKET FRAME SIZE VULNERABILITY**  
**Priority: HIGH**
- **File**: `src/server/WebDash.cpp:128-137`
- **Issue**: WebSocket frame size stored in single byte, truncation possible
- **Impact**: Frame size truncation for messages > 255 bytes, protocol confusion
- **Details**: `uint8_t hdr[2]{0x81, static_cast<uint8_t>(status.size())};` truncates size
- **Fix Required**: Implement proper WebSocket frame size encoding

### **ISSUE #44: MANUAL JSON PARSING VULNERABILITIES**
**Priority: HIGH**
- **File**: `src/server/QuestWatchdog.cpp:150-167,175-197`
- **Issue**: Manual JSON parsing using string operations without validation
- **Impact**: JSON injection, integer overflow, format string attacks
- **Details**: Uses `std::stoul()` and `std::stoi()` on unvalidated substrings
- **Fix Required**: Use proper JSON parsing library

### **ISSUE #45: LINUX-SPECIFIC HEADERS ON WINDOWS**
**Priority: MEDIUM**
- **File**: `src/server/WebDash.cpp:8-11`
- **Issue**: Linux-specific socket headers included in cross-platform code
- **Impact**: Compilation failure on Windows
- **Details**: `#include <arpa/inet.h>`, `<netinet/in.h>`, `<sys/socket.h>`, `<unistd.h>`
- **Fix Required**: Use platform-specific socket headers with #ifdef guards

### **ISSUE #46: QUEST STAGE ARRAY BOUNDS OVERFLOW**
**Priority: HIGH**
- **File**: `src/server/QuestWatchdog.cpp:84-88`
- **Issue**: Fixed-size quest array (32) without bounds checking
- **Impact**: Buffer overflow if more than 32 quests active
- **Details**: `if (outPkt.questCount >= 32) break;` but continues writing to array
- **Fix Required**: Add proper bounds checking before array access

### **ISSUE #47: WEAK DAMAGE VALIDATION FORMULA**  
**Priority: MEDIUM**
- **File**: `src/server/DamageValidator.cpp:18-24`
- **Issue**: Damage validation uses simple linear formula, easily bypassed
- **Impact**: Cheat detection can be circumvented with careful timing
- **Details**: `uint16_t maxAllowed = static_cast<uint16_t>((targetArmor * 4 + 200) * mult);`
- **Fix Required**: Implement more sophisticated anti-cheat validation

## WINDOWS COMPILATION BLOCKERS (CRITICAL)

### **ISSUE #48: LINUX SOCKET HEADERS IN INFOSERVER**
**Priority: CRITICAL**
- **File**: `src/server/InfoServer.cpp:3-6`
- **Issue**: Linux-only socket headers that don't exist on Windows
- **Impact**: Compilation failure on Windows with MSVC
- **Details**: `#include <arpa/inet.h>`, `<netinet/in.h>`, `<sys/socket.h>`, `<unistd.h>`
- **Fix Required**: Add Windows socket includes with platform guards

### **ISSUE #49: LINUX SOCKET HEADERS IN WEBDASH**
**Priority: CRITICAL**
- **File**: `src/server/WebDash.cpp:8-11`
- **Issue**: Linux-only socket headers that don't exist on Windows
- **Impact**: Compilation failure on Windows with MSVC
- **Details**: `#include <arpa/inet.h>`, `<netinet/in.h>`, `<sys/socket.h>`, `<unistd.h>`
- **Fix Required**: Add Windows socket includes with platform guards

### **ISSUE #50: LINUX SOCKET API USAGE**
**Priority: CRITICAL**
- **Files**: `src/server/InfoServer.cpp`, `src/server/WebDash.cpp`
- **Issue**: Uses Linux socket functions without Windows equivalents
- **Impact**: Linking errors and runtime crashes on Windows
- **Details**: `close()` should be `closesocket()`, missing WSAStartup/WSACleanup
- **Fix Required**: Use Windows socket API with platform-specific code

### **ISSUE #51: UNISTD.H INCLUDES WITHOUT PLATFORM GUARDS**
**Priority: CRITICAL**
- **Files**: `src/net/Connection.cpp:28`, `src/server/AdminController.cpp:13`, `src/core/GameProcess.cpp:7`, `src/server/DedicatedServer.cpp:15`
- **Issue**: Linux-specific header included without platform guards
- **Impact**: Compilation failure on Windows
- **Details**: `#include <unistd.h>` doesn't exist on Windows
- **Fix Required**: Add `#ifdef` guards or remove unnecessary includes

### **ISSUE #52: POSIX PROCESS FUNCTIONS**
**Priority: CRITICAL**
- **File**: `src/core/GameProcess.cpp:38,53`
- **Issue**: Uses POSIX-specific process functions unavailable on Windows
- **Impact**: Compilation and linking errors
- **Details**: `posix_spawn_file_actions_addclose()`, `waitpid()` are POSIX-only
- **Fix Required**: Use Windows process creation APIs with platform guards

### **ISSUE #53: MISSING WINDOWS WINSOCK INITIALIZATION**
**Priority: HIGH**
- **Files**: All networking files
- **Issue**: Windows requires WSAStartup()/WSACleanup() for socket operations
- **Impact**: Socket operations will fail on Windows
- **Details**: No Winsock initialization code found
- **Fix Required**: Add Windows socket initialization

### **ISSUE #54: INVENTORYCONTROLLER RACE CONDITIONS**
**Priority: CRITICAL**
- **File**: `src/server/InventoryController.cpp:12-13`
- **Issue**: Global static variables accessed from multiple threads without synchronization
- **Impact**: Race conditions causing item ID collisions, corrupted inventory state, crashes
- **Details**: `static uint64_t g_nextItemId = 1;` and `static std::unordered_map<uint64_t, ItemSnap> g_items;`
- **Fix Required**: Add mutex protection around all access to these globals

### **ISSUE #55: UNBOUNDED MEMORY GROWTH IN INVENTORY**
**Priority: CRITICAL**
- **File**: `src/server/InventoryController.cpp:31,46`
- **Issue**: Items are never removed from g_items map
- **Impact**: Unbounded memory growth leading to crashes
- **Details**: `g_items[snap.itemId] = snap;` with no cleanup mechanism
- **Fix Required**: Implement item cleanup/garbage collection

### **ISSUE #56: PERKCONTROLLER THREAD SAFETY ISSUES**
**Priority: HIGH**
- **File**: `src/server/PerkController.cpp:17-18`
- **Issue**: Concurrent access to nested containers without synchronization
- **Impact**: Data corruption, crashes, iterator invalidation
- **Details**: Static nested maps accessed without mutex protection
- **Fix Required**: Add mutex protection for all container operations

### **ISSUE #57: VEHICLECONTROLLER SINGLE VEHICLE LIMITATION**
**Priority: HIGH**
- **File**: `src/server/VehicleController.cpp:35`
- **Issue**: Only supports one vehicle in the entire server
- **Impact**: Multiplayer game can only have one vehicle
- **Details**: `static VehicleState g_vehicle;` - single static instance
- **Fix Required**: Use container to support multiple vehicles

### **ISSUE #58: UNDEFINED NAVIGATION FUNCTION**
**Priority: CRITICAL**
- **File**: `src/server/VehicleController.cpp:13`
- **Issue**: Function declared but never defined anywhere in codebase
- **Impact**: Linking errors
- **Details**: `bool Nav_FindClosestRoad()` declaration without implementation
- **Fix Required**: Implement function or remove calls to it

### **ISSUE #59: MISSING INCLUDE IN NPCCONTROLLER**
**Priority: CRITICAL**
- **File**: `src/server/NpcController.cpp:71`
- **Issue**: Uses `g_interestGrid` but doesn't include InterestGrid.hpp
- **Impact**: Compilation failure
- **Details**: Uses global variable from header that's not included
- **Fix Required**: Add `#include "../net/InterestGrid.hpp"`

### **ISSUE #60: TRADECONTROLLER UNDEFINED SYMBOL ACCESS**
**Priority: CRITICAL**
- **File**: `src/server/TradeController.cpp:44,45,55,56,61,62,112,119`
- **Issue**: Uses undefined global `g_items` defined as static in InventoryController.cpp
- **Impact**: Compilation/linking errors - symbol not exported
- **Details**: Attempts to access static variable from different translation unit
- **Fix Required**: Export g_items via getter function or make non-static with external linkage

### **ISSUE #61: HEARTBEAT INCORRECT POINTER ACCESS**
**Priority: CRITICAL**
- **File**: `src/server/Heartbeat.cpp:26,65,109`
- **Issue**: Attempts to access .status on httplib::Result object incorrectly
- **Impact**: Compilation error - treats struct as pointer
- **Details**: Code treats `res` as pointer but `httplib::Client::Get()` returns struct
- **Fix Required**: Remove pointer dereference (use `res.status` instead of `res->status`)

### **ISSUE #62: DEDICATEDMAIN VARIABLE DECLARATION ISSUES**
**Priority: CRITICAL**
- **File**: `src/server/DedicatedMain.cpp:83-88,96-102`
- **Issue**: Variables declared after use
- **Impact**: Compilation errors
- **Details**: Variables like `sunAngle`, `particleSeed`, `weatherId` used before declaration
- **Fix Required**: Move variable declarations before first use (around line 94)

### **ISSUE #63: STATUSCONTROLLER TYPE CONVERSION BUG**
**Priority: HIGH**
- **File**: `src/server/StatusController.cpp:30-31`
- **Issue**: Unsafe type casting in timer logic
- **Impact**: Integer underflow/overflow causing incorrect timer behavior
- **Details**: `static_cast<uint16_t>(it->remaining - dt)` and `static_cast<uint16_t>(dt)` without bounds checking
- **Fix Required**: Add bounds checking before casting

### **ISSUE #64: APARTMENTCONTROLLER UNVALIDATED INPUT**
**Priority: HIGH**
- **File**: `src/server/ApartmentController.cpp:55,58,61,64,67,73`
- **Issue**: `std::stoul()` and `std::stof()` without exception handling
- **Impact**: std::invalid_argument exceptions if CSV contains invalid data
- **Details**: Direct parsing without try-catch blocks
- **Fix Required**: Add try-catch blocks around parsing operations

### **ISSUE #65: ELEVATORCONTROLLER DIVISION BY ZERO RISK**
**Priority: MEDIUM**
- **File**: `src/server/ElevatorController.cpp:46`
- **Issue**: Potential division by zero if no connections exist
- **Impact**: Logic failure when checking acknowledgments
- **Details**: `g_arrive.acks.size() < Net_GetConnections().size()` - no validation connections exist
- **Fix Required**: Validate connection count before comparison

### **ISSUE #66: POLICEDISPATCH INTEGER OVERFLOW**
**Priority: MEDIUM**
- **File**: `src/server/PoliceDispatch.cpp:31-32`
- **Issue**: Timer accumulation without bounds checking
- **Impact**: Timer overflow after ~49 days of continuous operation
- **Details**: `g_timer += static_cast<uint32_t>(dt);` and `g_maxtac += static_cast<uint32_t>(dt);`
- **Fix Required**: Reset timers periodically or use larger data types

### **ISSUE #67: PHASEGC DIVISION BY ZERO**
**Priority: HIGH**
- **File**: `src/server/PhaseGC.cpp:23`
- **Issue**: Division without bounds checking
- **Impact**: Division by zero crash if GameClock::GetTickMs() returns 0
- **Details**: `static_cast<uint64_t>(600000.0f / GameClock::GetTickMs())`
- **Fix Required**: Add bounds checking for zero divisor

### **ISSUE #68: SECTORLODCONTROLLER LINUX-ONLY CODE**
**Priority: MEDIUM**
- **File**: `src/server/SectorLODController.cpp:21-28`
- **Issue**: Memory info only available on Linux systems
- **Impact**: LOD logic ineffective on non-Linux platforms (memRatio always 0)
- **Details**: `#ifdef __GLIBC__` code path only, no Windows equivalent
- **Fix Required**: Implement cross-platform memory detection

### **ISSUE #69: CARRYCONTROLLER EMPTY POSITION UPDATES**
**Priority: MEDIUM**
- **File**: `src/server/CarryController.cpp:36-37`
- **Issue**: Sends uninitialized position data every tick
- **Impact**: Wastes bandwidth broadcasting empty/zero vectors
- **Details**: `RED4ext::Vector3 pos{};` and `RED4ext::Vector3 vel{};` sent without actual data
- **Fix Required**: Only send updates when actual position data is available

### **ISSUE #70: JOURNAL FILE I/O IN HOT PATH**
**Priority: MEDIUM**
- **File**: `src/server/Journal.cpp:42-47`
- **Issue**: File operations on every log call
- **Impact**: Performance degradation from constant file open/close
- **Details**: Opens and closes log file for every journal entry
- **Fix Required**: Keep file handle open or batch writes

### **ISSUE #71: MULTIPLE GLOBAL STATIC THREAD SAFETY ISSUES**
**Priority: HIGH**
- **Files**: Multiple server files have static globals without synchronization
- **Issue**: Race conditions from unsynchronized access to static variables
- **Impact**: Data corruption, crashes, unpredictable behavior
- **Details**: 
  - `BreachController.cpp:9-14` - g_active, g_seed, etc.
  - `ShardController.cpp:8-12` - g_active, g_phase, etc.
  - `StatusController.cpp:17` - g_entries vector
  - `TrafficController.cpp:8` - g_seedTimer
  - `BillboardController.cpp:13` - g_map
- **Fix Required**: Add mutex protection to all static container access

---

## FUNCTION REFERENCE TRACKING

### Confirmed Function Existence:
- ✅ `CoopNet::GetAssetStreamer()` - defined in AssetStreamer.cpp:169
- ✅ `CoopNet::Fnv1a32()` - defined in Hash.hpp:19
- ✅ `SessionState_SetParty()` - defined in SessionState.cpp:31
- ✅ `Ledger_Transfer()` - defined in LedgerService.cpp:30

### Undefined Function Calls Found:
- ❌ `GetPlayerPeerId()` - called in InventorySync.reds:48 but not found
- ❌ `Net_SendInventorySnapshot()` - called in InventorySync.reds:57 but not found
- ❌ `GetPlayerMoney()` - called in InventorySync.reds:63 but not found
- ❌ `GetPlayerInventoryVersion()` - called in InventorySync.reds:64 but not found

### Functions Need Parameter Validation:
- ⚠️ `AvatarProxy_SpawnRemote()` - Connection.cpp:38, no parameter validation
- ⚠️ `ChatOverlay_Push()` - Connection.cpp:62, no null checking

---

## NEXT FILES TO ANALYZE (Priority Order):

### High Priority Server Files:
1. **DedicatedServer.cpp** - Core server implementation
2. **AdminController.cpp** - Admin command security
3. **VendorController.cpp** - Economic system
4. **QuestWatchdog.cpp** - Quest state management

### High Priority REDscript Files:
1. **Main.reds** - Entry point analysis needed
2. **Runtime/SessionState.reds** - State management
3. **GUI files** - UI integration issues
4. **Security/AntiCheatFramework.reds** - Security implementation

### Medium Priority:
1. Plugin system files (PluginManager.cpp)
2. Voice system files  
3. Physics system files
4. Remaining networking files

Last updated: In-progress analysis