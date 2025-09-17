// ============================================================================
// CP2077-COOP Native Function Declarations
// ============================================================================
// This file contains all native function declarations required by the 
// cp2077-coop mod to interface with the C++ backend.

// ============================================================================
// NETWORKING FUNCTIONS
// ============================================================================

// Core networking
public static native func Net_IsConnected() -> Bool;
public static native func Net_IsAuthoritative() -> Bool;
public static native func Net_GetPeerId() -> Uint32;
public static native func Net_Poll(maxMs: Uint32) -> Void;
public static native func Net_StartServer(port: Uint32, maxPlayers: Uint32) -> Bool;
public static native func Net_ConnectToServer(host: String, port: Uint32) -> Bool;
public static native func Net_Disconnect() -> Void;

// Server browser and connection
public static native func Net_SendJoinRequest(serverId: Uint32) -> Void;
public static native func Net_BroadcastServerInfo(info: String) -> Void;

// Breach protocol networking
public static native func Net_SendBreachInput(index: Uint8) -> Void;
public static native func Net_BroadcastBreachStart(peerId: Uint32, seed: Uint32, width: Uint8, height: Uint8) -> Void;
public static native func Net_BroadcastBreachResult(peerId: Uint32, result: Uint8) -> Void;

// AI and hacking
public static native func Net_BroadcastAIHack(targetId: Uint32, effectId: Uint8) -> Void;
public static native func Net_BroadcastQuickhack(casterId: Uint32, targetId: Uint32, hackId: Uint32) -> Void;

// Quest synchronization
public static native func Net_BroadcastQuestStage(questHash: Uint32, stage: Uint16) -> Void;
public static native func Net_BroadcastQuestStageP2P(phaseId: Uint32, questHash: Uint32, stage: Uint16) -> Void;
public static native func Net_SendQuestResync() -> Void;

// Cinematic and cutscene sync
public static native func Net_BroadcastCineStart(sceneId: Uint32, startTime: Uint32, phaseId: Uint32, solo: Bool) -> Void;
public static native func Net_BroadcastCutsceneSync(sceneId: Uint32, timestamp: Uint32) -> Void;

// Dialog system
public static native func Net_SendDialogChoice(choiceIndex: Uint8) -> Void;
public static native func Net_BroadcastDialogChoice(peerId: Uint32, choiceIndex: Uint8) -> Void;

// Vehicle networking
public static native func Net_BroadcastVehicleSpawn(vehicleId: Uint32, templateId: Uint32, pos: Vector3, rot: Quaternion) -> Void;
public static native func Net_BroadcastVehicleDestroy(vehicleId: Uint32) -> Void;
public static native func Net_SendVehicleSummon(vehicleId: Uint32, pos: Vector3) -> Void;

// World state
public static native func Net_BroadcastWorldState(sunAngle: Uint16, weatherId: Uint8, seed: Uint16) -> Void;
public static native func Net_BroadcastWeatherChange(weatherId: Uint8) -> Void;

// ============================================================================
// VOICE CHAT FUNCTIONS
// ============================================================================

public static native func CoopVoice_Initialize() -> Bool;
public static native func CoopVoice_Shutdown() -> Void;
public static native func CoopVoice_StartCapture() -> Bool;
public static native func CoopVoice_StopCapture() -> Void;
public static native func CoopVoice_EncodeFrame(inputData: array<Uint8>) -> array<Uint8>;
public static native func CoopVoice_DecodeFrame(encodedData: array<Uint8>) -> array<Uint8>;
public static native func CoopVoice_SetVolume(volume: Float) -> Void;
public static native func CoopVoice_GetVolume() -> Float;
public static native func CoopVoice_SetMuted(muted: Bool) -> Void;
public static native func CoopVoice_IsMuted() -> Bool;

// Voice networking
public static native func Net_SendVoice(data: array<Uint8>, size: Uint16) -> Void;
public static native func Net_BroadcastVoice(peerId: Uint32, data: array<Uint8>, size: Uint16) -> Void;
public static native func Net_BroadcastViseme(npcId: Uint32, visemeId: Uint8, timeMs: Uint32) -> Void;

// ============================================================================
// GAME STATE FUNCTIONS
// ============================================================================

// Session and player management
public static native func SessionState_GetActivePlayerCount() -> Uint32;
public static native func SessionState_GetPhaseId() -> Uint32;
public static native func SessionState_IsHost() -> Bool;
public static native func SessionState_GetPlayerId(index: Uint32) -> Uint32;

// Game clock and timing
public static native func GameClock_GetTime() -> Uint32;
public static native func GameClock_GetTickMs() -> Float;
public static native func GameClock_GetCurrentTick() -> Uint64;

// ============================================================================
// INVENTORY FUNCTIONS
// ============================================================================

public static native func Net_SendInventorySnapshot(peerId: Uint32, items: array<Uint64>, money: Uint64) -> Void;
public static native func Net_SendItemTransferRequest(fromPeer: Uint32, toPeer: Uint32, itemId: Uint64, quantity: Uint32) -> Void;
public static native func Net_SendItemPickup(itemId: Uint64, worldPosX: Float, worldPosY: Float, worldPosZ: Float, playerId: Uint32) -> Void;

// ============================================================================
// SYSTEM FUNCTIONS
// ============================================================================

// Process management
public static native func GameProcess_Launch(executable: String, arguments: String) -> Bool;
public static native func GameProcess_IsRunning(processId: Uint32) -> Bool;

// Hash and utility functions
public static native func Fnv1a32(input: String) -> Uint32;
public static native func Fnv1a64(input: String) -> Uint64;
public static native func Fnv1a64Pos(x: Float, y: Float) -> Uint64;

// HTTP requests
public static native func Http_SendAsync(url: String, method: String, data: String) -> Uint32;
public static native func Http_PollResponse(token: Uint32) -> ref<HttpResponse>;

// Settings and configuration
public static native func Config_SaveSettings(json: String) -> Bool;
public static native func Config_LoadSettings() -> String;

// ============================================================================
// PLUGIN SYSTEM
// ============================================================================

public static native func Plugin_SendRPC(pluginId: Uint16, functionHash: Uint32, jsonData: String) -> Void;
public static native func Plugin_BroadcastRPC(pluginId: Uint16, functionHash: Uint32, jsonData: String) -> Void;
public static native func Plugin_LoadAssetBundle(pluginId: Uint16, bundleData: array<Uint8>) -> Bool;

// ============================================================================
// EMOTE AND ANIMATION SYNC
// ============================================================================

public static native func Net_BroadcastEmote(peerId: Uint32, emoteId: Uint8) -> Void;
public static native func Net_BroadcastFinisherStart(actorId: Uint32, victimId: Uint32, finisherType: Uint8) -> Void;
public static native func Net_BroadcastFinisherEnd(actorId: Uint32) -> Void;
public static native func Net_BroadcastSlowMoFinisher(peerId: Uint32, victimId: Uint32, durationMs: Uint16) -> Void;

// ============================================================================
// APARTMENT SYSTEM
// ============================================================================

public static native func Net_SendApartmentPurchase(apartmentId: Uint32) -> Void;
public static native func Net_SendApartmentEnter(apartmentId: Uint32, ownerPhaseId: Uint32) -> Void;
public static native func Net_BroadcastApartmentState(phaseId: Uint32, jsonData: String) -> Void;

// ============================================================================
// COMBAT AND DAMAGE
// ============================================================================

public static native func Net_BroadcastDamage(attackerId: Uint32, victimId: Uint32, damage: Float, damageType: Uint8) -> Void;
public static native func Net_BroadcastStatusEffect(targetId: Uint32, effectId: Uint8, duration: Uint16, amplitude: Uint8) -> Void;

// ============================================================================
// PERK AND SKILL SYNC
// ============================================================================

public static native func Net_SendPerkUnlock(perkId: Uint32, rank: Uint8) -> Void;
public static native func Net_BroadcastPerkUnlock(peerId: Uint32, perkId: Uint32, rank: Uint8) -> Void;
public static native func Net_SendSkillXP(skillId: Uint16, deltaXP: Int16) -> Void;
public static native func Net_BroadcastSkillXP(peerId: Uint32, skillId: Uint16, deltaXP: Int16) -> Void;

// ============================================================================
// WORLD SYNC
// ============================================================================

public static native func Net_BroadcastPropBreak(entityId: Uint32, seed: Uint32) -> Void;
public static native func Net_BroadcastPropIgnite(entityId: Uint32, delayMs: Uint16) -> Void;
public static native func Net_BroadcastCrowdSeed(sectorHash: Uint64, seed: Uint32) -> Void;
public static native func Net_BroadcastTrafficSeed(sectorHash: Uint64, seed: Uint64) -> Void;

// ============================================================================
// DOOR BREACH SYSTEM
// ============================================================================

public static native func Net_DoorBreachController_Start(doorId: Uint32, phaseId: Uint32) -> Void;

// ============================================================================
// GAME MODE AND PHASE MANAGEMENT
// ============================================================================

public static native func Net_BroadcastRuleChange(friendlyFire: Bool) -> Void;
public static native func Net_AddStats(peerId: Uint32, frags: Uint16, deaths: Uint16, score: Uint32, timeMs: Uint32, ping: Uint16) -> Void;
public static native func Net_BroadcastMatchOver(winnerId: Uint32) -> Void;
public static native func Net_BroadcastGigSpawn(questId: Uint32, seed: Uint32) -> Void;
public static native func Net_BroadcastHeat(level: Uint8) -> Void;
public static native func Net_SendElevatorCall(elevatorId: Uint32, floor: Uint8) -> Void;
public static native func Net_SendTeleportAck(elevatorId: Uint32) -> Void;
public static native func Net_BroadcastLootRoll(containerId: Uint32, seed: Uint32, items: array<Uint64>) -> Void;
public static native func Net_GetPeerCount() -> Uint32;

// ============================================================================
// NPC AND AI MANAGEMENT
// ============================================================================

public static native func Net_NpcController_ServerTick(deltaTime: Float) -> Void;
public static native func Net_SpawnPhaseNpc() -> Void;
public static native func Net_BroadcastNpcSpawnCruiser(waveIdx: Uint8, seeds: array<Uint32>) -> Void;

// ============================================================================
// ERROR HANDLING
// ============================================================================

public static native func Native_GetLastError() -> String;
public static native func Native_ClearError() -> Void;
public static native func Native_IsInitialized() -> Bool;

// ============================================================================
// HELPER STRUCTS FOR COMPLEX NATIVE CALLS
// ============================================================================

public struct HttpResponse {
    public var token: Uint32;
    public var status: Uint16;
    public var body: String;
    public var headers: String;
}

public struct NetworkPacket {
    public var type: Uint16;
    public var size: Uint16;
    public var data: array<Uint8>;
}

public struct ItemSnapshot {
    public var itemId: Uint64;
    public var quantity: Uint32;
    public var quality: Uint32;
    public var mods: array<Uint64>;
}

public struct PlayerSnapshot {
    public var peerId: Uint32;
    public var position: Vector3;
    public var rotation: Quaternion;
    public var health: Float;
    public var money: Uint64;
    public var level: Uint32;
}

// ============================================================================
// INITIALIZATION AND CLEANUP
// ============================================================================

// Called by the native backend to initialize the REDscript interface
public static native func CoopNative_Initialize() -> Bool;
public static native func CoopNative_Shutdown() -> Void;
public static native func CoopNative_GetVersion() -> String;
public static native func CoopNative_IsCompatible(gameVersion: String) -> Bool;