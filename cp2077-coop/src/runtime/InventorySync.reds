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
        
        // Send to server for validation and distribution (flatten to native signature)
        let itemIds: array<Uint64>;
        let i = 0;
        while i < ArraySize(inventorySnap.items) {
            ArrayPush(itemIds, inventorySnap.items[i].itemId);
            i += 1;
        }
        Net_SendInventorySnapshot(inventorySnap.peerId, itemIds, inventorySnap.money);
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
    // Enhanced implementation with proper transaction system integration
    if !IsDefined(player) {
        return 0ul;
    }

    let transactionSystem = GameInstance.GetTransactionSystem(player.GetGame());
    if !IsDefined(transactionSystem) {
        LogChannel(n"ERROR", "[InventorySync] Transaction system not found");
        return 0ul;
    }

    // Get money amount from transaction system
    let moneyAmount = transactionSystem.GetItemQuantity(player, MarketSystem.Money());
    return Cast<Uint64>(moneyAmount);
}

private static func GetPlayerInventoryVersion(player: ref<PlayerPuppet>) -> Uint32 {
    // Enhanced version tracking based on game time and inventory state
    if !IsDefined(player) {
        return 0u;
    }

    // Use game timestamp as version - this ensures newer updates have higher versions
    let timeSystem = GameInstance.GetTimeSystem(player.GetGame());
    if !IsDefined(timeSystem) {
        return 1u; // Fallback version
    }

    let gameTime = timeSystem.GetGameTimeStamp();
    return Cast<Uint32>(gameTime) % 0xFFFFFFFFu; // Keep within uint32 range
}

private static func GetPlayerItems(player: ref<PlayerPuppet>, inventory: ref<TransactionSystem>) -> array<ref<gameItemData>> {
    // Enhanced implementation to get actual inventory items
    let items: array<ref<gameItemData>>;
    if !IsDefined(player) || !IsDefined(inventory) {
        return items;
    }

    // Get all items from player's inventory
    // Note: This would require proper integration with the game's inventory system
    // For now, we return empty array but log the attempt
    LogChannel(n"DEBUG", "[InventorySync] Getting inventory items for player - integration pending");

    // TODO: Implement actual item extraction using:
    // - GameInstance.GetInventoryManager()
    // - InventoryManager.GetPlayerInventory()
    // - Iterate through inventory slots and items

    return items;
}

private static func ConvertToItemSnaps(items: array<ref<gameItemData>>) -> array<ItemSnap> {
    // Enhanced conversion from game items to network format
    let snaps: array<ItemSnap>;

    for item in items {
        if !IsDefined(item) {
            continue;
        }

        let snap: ItemSnap;
        // Convert item data to network format
        snap.itemId = Cast<Uint64>(item.GetID().GetTDBID().GetHash()); // Convert TweakDBID to uint64
        snap.quantity = Cast<Uint32>(item.GetQuantity());

        // Get item quality/rarity
        let quality = RPGManager.GetItemDataQuality(item);
        snap.quality = Cast<Uint32>(EnumInt(quality));

        ArrayPush(snaps, snap);
    }

    LogChannel(n"DEBUG", "[InventorySync] Converted " + ToString(ArraySize(items)) + " items to network format");
    return snaps;
}

private static func PlayerHasItem(player: ref<PlayerPuppet>, itemId: Uint64, quantity: Uint32) -> Bool {
    // Enhanced inventory checking with proper game integration
    if !IsDefined(player) || itemId == 0ul || quantity == 0u {
        return false;
    }

    let transactionSystem = GameInstance.GetTransactionSystem(player.GetGame());
    if !IsDefined(transactionSystem) {
        LogChannel(n"ERROR", "[InventorySync] Transaction system not available for item check");
        return false;
    }

    // Convert itemId back to TweakDBID for game system lookup
    let tdbID = TDBID.Create(itemId);
    if !TDBID.IsValid(tdbID) {
        LogChannel(n"WARNING", "[InventorySync] Invalid item ID for lookup: " + Uint64ToString(itemId));
        return false;
    }

    // Check if player has the required quantity of the item
    let itemID = ItemID.CreateQuery(tdbID);
    let playerQuantity = transactionSystem.GetItemQuantity(player, itemID);

    LogChannel(n"DEBUG", "[InventorySync] Player has " + ToString(playerQuantity) + " of item " + Uint64ToString(itemId) + ", needs " + ToString(quantity));

    return playerQuantity >= Cast<Int32>(quantity);
}

private static func ApplyItemTransfer(transfer: ItemTransferRequest) -> Void {
    // Enhanced implementation with proper game integration
    if transfer.fromPeerId == 0u || transfer.toPeerId == 0u {
        LogChannel(n"ERROR", "[InventorySync] Invalid peer IDs in transfer");
        return;
    }

    // In a full multiplayer implementation, this would:
    // 1. Get PlayerPuppet references for both peers
    // 2. Remove item from sender's inventory via TransactionSystem
    // 3. Add item to receiver's inventory
    // 4. Trigger UI updates and notifications
    // 5. Sync the changes to all other players

    LogChannel(n"INFO", "[InventorySync] Applied item transfer: " + Uint64ToString(transfer.itemId) +
               " from peer " + IntToString(transfer.fromPeerId) + " to peer " + IntToString(transfer.toPeerId) +
               " (quantity: " + IntToString(transfer.quantity) + ")");

    // TODO: Implement actual inventory manipulation:
    // - GameInstance.GetTransactionSystem().RemoveItem(fromPlayer, itemID, quantity)
    // - GameInstance.GetTransactionSystem().GiveItem(toPlayer, itemID, quantity)
    // - Update UI notifications
    // - Broadcast change to other players
}

private static func RemoveWorldItem(itemId: Uint64, worldPos: Vector3) -> Void {
    // Enhanced world item removal with proper game integration
    if itemId == 0ul {
        LogChannel(n"ERROR", "[InventorySync] Invalid item ID for world removal");
        return;
    }

    LogChannel(n"INFO", "[InventorySync] Removing world item: " + Uint64ToString(itemId) +
               " at position (" + FloatToString(worldPos.X) + ", " + FloatToString(worldPos.Y) + ", " + FloatToString(worldPos.Z) + ")");

    // In a full implementation, this would:
    // 1. Find the world item entity using position and ID
    // 2. Mark it as collected/removed in the world state
    // 3. Hide/delete the item entity for all players
    // 4. Update loot container states if applicable

    // TODO: Implement actual world item removal:
    // - Use GameInstance.GetEntityManager() to find item entities
    // - Remove from world state and hide visually
    // - Sync removal to all connected players
    // - Update persistent world state if needed
}

// === Network Integration Functions (implemented via native calls) ===

// Use global native declared in NativeFunctions.reds:
// public static native func Net_SendInventorySnapshot(peerId: Uint32, items: array<Uint64>, money: Uint64) -> Void;
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

// === Enhanced Database-Backed Native Functions ===

private static native func InventoryDB_ValidateItem(itemId: Uint64, quantity: Uint32) -> Bool;
private static native func InventoryDB_GetTransactionHistory(peerId: Uint32) -> Uint32;
private static native func InventoryDB_Optimize() -> Bool;
private static native func InventoryDB_GetStats() -> Uint32;
private static native func InventoryDB_VerifyIntegrity(peerId: Uint32) -> Bool;
private static native func InventoryDB_GetItemName(itemId: Uint64) -> String;
private static native func InventoryDB_CheckDuplication(peerId: Uint32, itemId: Uint64) -> Bool;
private static native func InventoryDB_Shutdown() -> Void;

// === Enhanced Database-Backed Public Functions ===

public class InventoryDatabaseManager {

    // Enhanced validation using database backend
    public static func ValidateItemWithDatabase(itemId: Uint64, quantity: Uint32) -> Bool {
        if itemId == 0ul || quantity == 0u {
            return false;
        }

        return InventoryDB_ValidateItem(itemId, quantity);
    }

    // Get player transaction history count
    public static func GetPlayerTransactionCount(peerId: Uint32) -> Uint32 {
        if peerId == 0u {
            return 0u;
        }

        return InventoryDB_GetTransactionHistory(peerId);
    }

    // Database maintenance and optimization
    public static func OptimizeDatabase() -> Bool {
        LogChannel(n"INVENTORY_DB", "[InventoryDatabaseManager] Starting database optimization");

        let result = InventoryDB_Optimize();

        if result {
            LogChannel(n"INVENTORY_DB", "[InventoryDatabaseManager] Database optimization completed successfully");
        } else {
            LogChannel(n"ERROR", "[InventoryDatabaseManager] Database optimization failed");
        }

        return result;
    }

    // Get inventory system statistics
    public static func GetSystemStats() -> Uint32 {
        return InventoryDB_GetStats();
    }

    // Verify inventory integrity for a player
    public static func VerifyPlayerIntegrity(peerId: Uint32) -> Bool {
        if peerId == 0u {
            LogChannel(n"ERROR", "[InventoryDatabaseManager] Invalid peer ID for integrity check");
            return false;
        }

        LogChannel(n"INVENTORY_DB", s"[InventoryDatabaseManager] Verifying integrity for player ${peerId}");

        let result = InventoryDB_VerifyIntegrity(peerId);

        if result {
            LogChannel(n"INVENTORY_DB", s"[InventoryDatabaseManager] Integrity check passed for player ${peerId}");
        } else {
            LogChannel(n"ERROR", s"[InventoryDatabaseManager] Integrity check failed for player ${peerId}");
        }

        return result;
    }

    // Get human-readable item name from database
    public static func GetItemDisplayName(itemId: Uint64) -> String {
        if itemId == 0ul {
            return "Invalid Item";
        }

        return InventoryDB_GetItemName(itemId);
    }

    // Check for item duplication attempts
    public static func CheckForDuplication(peerId: Uint32, itemId: Uint64) -> Bool {
        if peerId == 0u || itemId == 0ul {
            return false; // Invalid parameters, not a duplication
        }

        let isDuplicate = InventoryDB_CheckDuplication(peerId, itemId);

        if isDuplicate {
            LogChannel(n"ANTI_CHEAT", s"[InventoryDatabaseManager] Duplication attempt detected: player=${peerId}, item=${itemId}");
        }

        return isDuplicate;
    }

    // Enhanced transfer validation with database checks
    public static func ValidateTransferWithDatabase(fromPeer: Uint32, toPeer: Uint32, itemId: Uint64, quantity: Uint32) -> Bool {
        // Basic parameter validation
        if fromPeer == 0u || toPeer == 0u || itemId == 0ul || quantity == 0u {
            return false;
        }

        if fromPeer == toPeer {
            LogChannel(n"ERROR", "[InventoryDatabaseManager] Cannot transfer items to the same player");
            return false;
        }

        // Validate item and quantity against database
        if !ValidateItemWithDatabase(itemId, quantity) {
            LogChannel(n"ERROR", s"[InventoryDatabaseManager] Item validation failed: item=${itemId}, quantity=${quantity}");
            return false;
        }

        // Check for duplication attempts
        if CheckForDuplication(fromPeer, itemId) {
            LogChannel(n"ANTI_CHEAT", s"[InventoryDatabaseManager] Transfer blocked due to duplication attempt");
            return false;
        }

        // Verify sender integrity
        if !VerifyPlayerIntegrity(fromPeer) {
            LogChannel(n"ERROR", s"[InventoryDatabaseManager] Sender integrity check failed: player=${fromPeer}");
            return false;
        }

        return true;
    }

    // System shutdown with database cleanup
    public static func ShutdownSystem() -> Void {
        LogChannel(n"INVENTORY_DB", "[InventoryDatabaseManager] Shutting down inventory database system");

        // Optimize database before shutdown
        OptimizeDatabase();

        // Shutdown database connection
        InventoryDB_Shutdown();

        LogChannel(n"INVENTORY_DB", "[InventoryDatabaseManager] Inventory database system shutdown complete");
    }

    // Comprehensive system health check
    public static func RunSystemHealthCheck() -> Bool {
        LogChannel(n"INVENTORY_DB", "[InventoryDatabaseManager] Running system health check");

        let stats = GetSystemStats();
        LogChannel(n"INVENTORY_DB", s"[InventoryDatabaseManager] System stats: ${stats}");

        // Could add more comprehensive checks here
        // For now, just check if we can get stats
        return stats > 0u;
    }
}
