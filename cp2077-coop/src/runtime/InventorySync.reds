// Comprehensive inventory synchronization system for multiplayer
public struct ItemSnap {
    public var itemId: Uint64;
    public var quantity: Uint32;
    public var quality: Uint32;
}

public struct PlayerInventorySnap {
    public var peerId: Uint32;
    public var items: array<ItemSnap>;
    public var money: Uint64;
    public var version: Uint32; // For conflict resolution
}

public struct ItemTransferRequest {
    public var fromPeerId: Uint32;
    public var toPeerId: Uint32;
    public var itemId: Uint64;
    public var quantity: Uint32;
    public var requestId: Uint32;
}

public struct ItemPickupEvent {
    public var itemId: Uint64;
    public var worldPosition: Vector3;
    public var playerId: Uint32;
    public var timestamp: Uint32;
}

public class InventorySync {
    // Player inventory states
    private static var playerInventories: array<PlayerInventorySnap>;
    
    // Pending transfers
    private static var pendingTransfers: array<ItemTransferRequest>;
    private static var nextRequestId: Uint32 = 1u;
    
    // Shared world items (loot containers, etc.)
    private static var worldItems: array<ItemPickupEvent>;
    
    // System state
    private static var isInitialized: Bool = false;
    private static var maxPlayers: Int32 = 32;
    
    // Configuration
    private static let MAX_TRANSFER_DISTANCE: Float = 10.0; // meters
    private static let TRANSFER_TIMEOUT: Uint32 = 30000u; // 30 seconds
    private static let MAX_PENDING_TRANSFERS: Int32 = 10;
    
    // === System Initialization ===
    
    public static func InitializeInventorySync(playerCount: Int32) -> Bool {
        LogChannel(n"INVENTORY_SYNC", s"Initializing inventory sync system for " + ToString(playerCount) + " players");
        
        if isInitialized {
            LogChannel(n"INVENTORY_SYNC", s"Inventory sync already initialized");
            return true;
        }
        
        maxPlayers = playerCount;
        
        // Initialize data structures
        ArrayClear(playerInventories);
        ArrayClear(pendingTransfers);
        ArrayClear(worldItems);
        nextRequestId = 1u;
        
        // Initialize native backend
        if !InventorySync_Initialize(playerCount) {
            LogChannel(n"INVENTORY_SYNC", s"Failed to initialize native inventory sync backend");
            return false;
        }
        
        // Start cleanup timer
        if !StartCleanupTimer() {
            LogChannel(n"INVENTORY_SYNC", s"Failed to start cleanup timer");
            return false;
        }
        
        isInitialized = true;
        LogChannel(n"INVENTORY_SYNC", s"Inventory sync system initialized successfully");
        return true;
    }
    
    // === Player Inventory Management ===
    
    public static func SyncPlayerInventory(player: ref<PlayerPuppet>) -> Void {
        if !IsDefined(player) {
            LogChannel(n"ERROR", "SyncPlayerInventory: Invalid player");
            return;
        }
        
        let peerId = GetPlayerPeerId(player);
        if peerId == 0u {
            return; // Local player or invalid
        }
        
        let inventorySnap = BuildInventorySnapshot(player);
        OnPlayerInventoryUpdate(inventorySnap);
        
        // Send to server for validation and distribution
        Net_SendInventorySnapshot(inventorySnap);
    }
    
    // === Missing Helper Functions Implementation ===
    
    private static func GetPlayerPeerId(player: ref<PlayerPuppet>) -> Uint32 {
        if !IsDefined(player) {
            return 0u;
        }
        // For now return 1 for local player, 0 for others
        // In full implementation this would get the actual peer ID
        return 1u;
    }
    
    private static func GetPlayerMoney(player: ref<PlayerPuppet>) -> Uint64 {
        if !IsDefined(player) {
            return 0ul;
        }
        
        let transactionSystem = GameInstance.GetTransactionSystem(player.GetGame());
        if !IsDefined(transactionSystem) {
            return 0ul;
        }
        
        return Cast<Uint64>(transactionSystem.GetItemQuantity(player, MarketSystem.Money()));
    }
    
    private static func GetPlayerInventoryVersion(player: ref<PlayerPuppet>) -> Uint32 {
        if !IsDefined(player) {
            return 0u;
        }
        // Simple version based on current time
        // In full implementation this would track actual inventory changes
        return Cast<Uint32>(GameInstance.GetTimeSystem(player.GetGame()).GetGameTimeStamp());
    }

    private static func BuildInventorySnapshot(player: ref<PlayerPuppet>) -> PlayerInventorySnap {
        let snap: PlayerInventorySnap;
        snap.peerId = GetPlayerPeerId(player);
        snap.money = GetPlayerMoney(player);
        snap.version = GetPlayerInventoryVersion(player);
        
        // Build item list from player's inventory
        let inventory = GameInstance.GetTransactionSystem(GetGame());
        if IsDefined(inventory) {
            let playerItems = GetPlayerItems(player, inventory);
            snap.items = ConvertToItemSnaps(playerItems);
        }
        
        return snap;
    }
    
    public static func OnPlayerInventoryUpdate(snap: PlayerInventorySnap) -> Void {
        let playerIndex = FindPlayerInventoryIndex(snap.peerId);
        
        if playerIndex >= 0 {
            // Version check for conflict resolution
            if snap.version <= playerInventories[playerIndex].version {
                LogChannel(n"WARNING", "Received outdated inventory update for peer " + IntToString(snap.peerId));
                return;
            }
            playerInventories[playerIndex] = snap;
        } else {
            // New player inventory
            ArrayPush(playerInventories, snap);
        }
        
        LogChannel(n"INFO", "Updated inventory for peer " + IntToString(snap.peerId) + 
                  " (" + IntToString(ArraySize(snap.items)) + " items)");
    }
    
    // === Item Transfer System ===
    
    public static func RequestItemTransfer(fromPlayer: ref<PlayerPuppet>, toPlayer: ref<PlayerPuppet>, 
                                          itemId: Uint64, quantity: Uint32) -> Bool {
        if !ValidateTransferRequest(fromPlayer, toPlayer, itemId, quantity) {
            return false;
        }
        
        if ArraySize(pendingTransfers) >= MAX_PENDING_TRANSFERS {
            LogChannel(n"ERROR", "Too many pending transfers, rejecting request");
            return false;
        }
        
        let request: ItemTransferRequest;
        request.fromPeerId = GetPlayerPeerId(fromPlayer);
        request.toPeerId = GetPlayerPeerId(toPlayer);
        request.itemId = itemId;
        request.quantity = quantity;
        request.requestId = nextRequestId++;
        
        ArrayPush(pendingTransfers, request);
        
        // Send to server for processing
        Net_SendItemTransferRequest(request);
        
        LogChannel(n"INFO", "Requested item transfer: " + Uint64ToString(itemId) + 
                  " from " + IntToString(request.fromPeerId) + " to " + IntToString(request.toPeerId));
        
        return true;
    }
    
    private static func ValidateTransferRequest(fromPlayer: ref<PlayerPuppet>, toPlayer: ref<PlayerPuppet>,
                                               itemId: Uint64, quantity: Uint32) -> Bool {
        if !IsDefined(fromPlayer) || !IsDefined(toPlayer) {
            LogChannel(n"ERROR", "Invalid players in transfer request");
            return false;
        }
        
        if quantity == 0u {
            LogChannel(n"ERROR", "Invalid quantity in transfer request");
            return false;
        }
        
        // Check distance between players
        let distance = Vector4.Distance(fromPlayer.GetWorldPosition(), toPlayer.GetWorldPosition());
        if distance > MAX_TRANSFER_DISTANCE {
            LogChannel(n"WARNING", "Players too far apart for item transfer: " + FloatToString(distance) + "m");
            return false;
        }
        
        // Check if fromPlayer actually has the item
        if !PlayerHasItem(fromPlayer, itemId, quantity) {
            LogChannel(n"WARNING", "Player does not have requested item for transfer");
            return false;
        }
        
        return true;
    }
    
    public static func OnItemTransferResponse(requestId: Uint32, success: Bool, reason: String) -> Void {
        let transferIndex = FindPendingTransferIndex(requestId);
        if transferIndex < 0 {
            LogChannel(n"WARNING", "Received transfer response for unknown request: " + IntToString(requestId));
            return;
        }
        
        // Add bounds check before accessing array
        if transferIndex >= ArraySize(pendingTransfers) {
            LogChannel(n"ERROR", "Transfer index out of bounds: " + IntToString(transferIndex));
            return;
        }
        
        let transfer = pendingTransfers[transferIndex];
        ArrayRemove(pendingTransfers, transfer);
        
        if success {
            LogChannel(n"INFO", "Item transfer completed: " + Uint64ToString(transfer.itemId));
            // Update local inventories if needed
            ApplyItemTransfer(transfer);
        } else {
            LogChannel(n"WARNING", "Item transfer failed: " + reason);
        }
    }
    
    // === World Item Management ===
    
    public static func OnWorldItemPickup(itemId: Uint64, worldPos: Vector3, playerId: Uint32) -> Void {
        // Check if item was already picked up by another player
        if IsWorldItemTaken(itemId) {
            LogChannel(n"WARNING", "Attempted to pickup already taken item: " + Uint64ToString(itemId));
            return;
        }
        
        let pickup: ItemPickupEvent;
        pickup.itemId = itemId;
        pickup.worldPosition = worldPos;
        pickup.playerId = playerId;
        pickup.timestamp = GameClock.GetTime();
        
        ArrayPush(worldItems, pickup);
        
        // Notify server of pickup
        Net_SendItemPickup(pickup);
        
        // Remove item from world for all players
        RemoveWorldItem(itemId, worldPos);
        
        LogChannel(n"INFO", "Item picked up: " + Uint64ToString(itemId) + " by player " + IntToString(playerId));
    }
    
    // === Utility Functions ===
    
    private static func FindPlayerInventoryIndex(peerId: Uint32) -> Int32 {
        let count = ArraySize(playerInventories);
        var i = 0;
        while i < count {
            if playerInventories[i].peerId == peerId {
                return i;
            }
            i += 1;
        }
        return -1;
    }
    
    private static func FindPendingTransferIndex(requestId: Uint32) -> Int32 {
        let count = ArraySize(pendingTransfers);
        var i = 0;
        while i < count {
            if pendingTransfers[i].requestId == requestId {
                return i;
            }
            i += 1;
        }
        return -1;
    }
    
    private static func IsWorldItemTaken(itemId: Uint64) -> Bool {
        let count = ArraySize(worldItems);
        var i = 0;
        while i < count {
            if worldItems[i].itemId == itemId {
                return true;
            }
            i += 1;
        }
        return false;
    }
    
    public static func CleanupExpiredRequests() -> Void {
        let currentTime = GameClock.GetTime();
        
        // Collect items to remove instead of modifying array during iteration
        let toRemove: array<ItemTransferRequest>;
        let maxAllowed = MAX_PENDING_TRANSFERS;
        
        // If we have too many pending transfers, remove oldest ones
        if ArraySize(pendingTransfers) > maxAllowed {
            let excessCount = ArraySize(pendingTransfers) - maxAllowed;
            var i = 0;
            while i < excessCount && i < ArraySize(pendingTransfers) {
                ArrayPush(toRemove, pendingTransfers[i]);
                i += 1;
            }
        }
        
        // Remove collected items safely
        for item in toRemove {
            ArrayRemove(pendingTransfers, item);
            LogChannel(n"WARNING", "Removed old transfer request due to overflow: " + IntToString(item.requestId));
        }
    }
    
    // === Integration with Game Systems ===
    
    public static func GetPlayerInventory(peerId: Uint32) -> ref<PlayerInventorySnap> {
        let index = FindPlayerInventoryIndex(peerId);
        if index >= 0 {
            return playerInventories[index];
        }
        return null;
    }
    
    public static func GetPlayerItemCount(peerId: Uint32, itemId: Uint64) -> Uint32 {
        let inventory = GetPlayerInventory(peerId);
        if !IsDefined(inventory) {
            return 0u;
        }
        
        let count = ArraySize(inventory.items);
        var i = 0;
        while i < count {
            if inventory.items[i].itemId == itemId {
                return 1u; // Simplified - would need actual quantity tracking
            }
            i += 1;
        }
        return 0u;
    }
    
    public static func ClearAllInventories() -> Void {
        ArrayClear(playerInventories);
        ArrayClear(pendingTransfers);
        ArrayClear(worldItems);
        nextRequestId = 1u;
    }
}

// === Helper functions with placeholder implementations ===

private static func GetPlayerPeerId(player: ref<PlayerPuppet>) -> Uint32 {
    // Simple placeholder - would integrate with networking system to get actual peer ID
    if !IsDefined(player) {
        return 0u;
    }
    // For now, return a non-zero value for any valid player
    return 1u;
}

private static func GetPlayerMoney(player: ref<PlayerPuppet>) -> Uint64 {
    // Simple placeholder - would integrate with transaction system
    if !IsDefined(player) {
        return 0u;
    }
    // Try to get money from transaction system
    let transactionSystem = GameInstance.GetTransactionSystem(GetGame());
    if IsDefined(transactionSystem) {
        // Would use actual money getter here
        return 1000u; // Placeholder amount
    }
    return 0u;
}

private static func GetPlayerInventoryVersion(player: ref<PlayerPuppet>) -> Uint32 {
    // Simple placeholder - would track inventory modification version
    if !IsDefined(player) {
        return 0u;
    }
    return 1u;
}

private static func GetPlayerItems(player: ref<PlayerPuppet>, inventory: ref<TransactionSystem>) -> array<ref<gameItemData>> {
    // Simple placeholder - would extract actual items from player inventory
    let items: array<ref<gameItemData>>;
    if !IsDefined(player) || !IsDefined(inventory) {
        return items;
    }
    // Would populate with actual inventory items
    return items;
}

private static func ConvertToItemSnaps(items: array<ref<gameItemData>>) -> array<ItemSnap> {
    // Simple placeholder - would convert game items to network format
    let snaps: array<ItemSnap>;
    // Would iterate through items and convert each to ItemSnap
    return snaps;
}

private static func PlayerHasItem(player: ref<PlayerPuppet>, itemId: Uint64, quantity: Uint32) -> Bool {
    // Simple placeholder - would check player's actual inventory
    if !IsDefined(player) || itemId == 0u || quantity == 0u {
        return false;
    }
    // Would check actual inventory here
    return false; // Always false for testing
}

private static func ApplyItemTransfer(transfer: ItemTransferRequest) -> Void {
    // Simple placeholder - would actually move items between player inventories
    if transfer.fromPeerId == 0u || transfer.toPeerId == 0u {
        LogChannel(n"ERROR", "Invalid peer IDs in transfer");
        return;
    }
    LogChannel(n"INFO", "Applied item transfer: " + Uint64ToString(transfer.itemId));
}

private static func RemoveWorldItem(itemId: Uint64, worldPos: Vector3) -> Void {
    // Simple placeholder - would remove item from world for all players
    if itemId == 0u {
        LogChannel(n"ERROR", "Invalid item ID for world removal");
        return;
    }
    LogChannel(n"INFO", "Removed world item: " + Uint64ToString(itemId));
}

// === Network Integration Functions (implemented via native calls) ===

private static native func Net_SendInventorySnapshot(snap: PlayerInventorySnap) -> Void;
private static native func Net_SendItemTransferRequest(request: ItemTransferRequest) -> Void;
private static native func Net_SendItemPickup(pickup: ItemPickupEvent) -> Void;

// === Native C++ Integration Functions ===

    private static func StartCleanupTimer() -> Bool {
        LogChannel(n"INVENTORY_SYNC", s"Starting cleanup timer for expired requests");
        // In a real implementation, this would set up a timer to periodically call CleanupExpiredRequests()
        return true;
    }

// === Native C++ Integration Functions ===

private static native func InventorySync_Initialize(maxPlayers: Int32) -> Bool;
private static native func InventorySync_UpdatePlayerInventory(peerId: Uint32, version: Uint32, money: Uint64) -> Bool;
private static native func InventorySync_RequestTransfer(fromPeer: Uint32, toPeer: Uint32, itemId: Uint64, quantity: Uint32) -> Uint32;
private static native func InventorySync_RegisterPickup(itemId: Uint64, posX: Float, posY: Float, posZ: Float, playerId: Uint32) -> Bool;
private static native func InventorySync_IsItemTaken(itemId: Uint64) -> Bool;
private static native func InventorySync_ProcessTransfer(requestId: Uint32, approve: Bool, reason: String) -> Bool;
private static native func InventorySync_GetPlayerCount() -> Uint32;
private static native func InventorySync_Cleanup() -> Void;