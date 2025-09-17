# Implementation Notes - Stubbed Functions

## Overview
This document tracks functions that have been stubbed during compilation fixes and need full implementation later.

## Stubbed Functions in Connection.cpp

### Controller Functions (Need proper server-side logic)
- `PerkController_HandleUnlock()` - Handle perk unlock requests from clients
- `PerkController_HandleRespec()` - Handle perk respec requests  
- `SkillController_HandleXP()` - Handle skill XP updates from clients
- `ElevatorController_OnCall()` - Handle elevator call requests
- `ElevatorController_OnAck()` - Handle elevator acknowledgments

### Data Building Functions (Need actual implementation)
- `BuildMarkerBlob()` - Should build world marker data blob for synchronization
- `BuildPhaseBundle()` - Should build phase-specific data bundle for quest sync

### NAT Traversal Functions (Need libjuice integration)
- `Nat_AddRemoteCandidate()` - Add remote NAT candidate for P2P connection
- `Nat_PerformHandshake()` - Perform NAT traversal handshake

### Missing Dependency Issues
- SaveMigration.cpp - Disabled rapidjson functionality (save migration disabled)
- NatTraversal.cpp - Missing libjuice library integration
- Various controller headers missing full implementations

## REDScript Bridge Functions (Using RED4EXT_EXECUTE)
These are properly implemented as bridges but may need parameter validation:
- CutsceneSync_* functions
- CyberwareSync_* functions  
- VehicleProxy_* functions
- AvatarProxy_* functions
- BillboardController_* functions
- QuickhackSync_* functions

## Next Steps for Full Implementation
1. Implement proper server-side controller logic
2. Add libjuice NAT traversal library
3. Add rapidjson for save migration features
4. Add parameter validation for all bridge functions
5. Implement proper error handling and logging
6. Add thread safety for multi-client scenarios

## Build Status
- Core compilation: ✅ FIXED
- Basic functionality: ✅ WORKING
- Advanced features: ⚠️ STUBBED (functional but limited)