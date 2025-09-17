// Shared Virtual Netrunning Environments for Cyberpunk 2077 Multiplayer
// Persistent cyberspace with competitive hacking and collaborative netrunning

module VirtualEnvironments

enum VirtualSpaceType {
    DataFortress = 0,
    CorporateNetwork = 1,
    PublicNet = 2,
    PrivateGrid = 3,
    DeepNet = 4,
    TrainingGround = 5,
    BattleArena = 6,
    MarketSpace = 7,
    SocialHub = 8,
    DarkWeb = 9
}

enum AccessLevel {
    Guest = 0,
    Authenticated = 1,
    Authorized = 2,
    Administrator = 3,
    SysAdmin = 4,
    Root = 5
}

enum NodeType {
    Data = 0,
    ICE = 1,
    Gateway = 2,
    Storage = 3,
    Process = 4,
    AI = 5,
    Daemon = 6,
    Firewall = 7,
    Honeypot = 8,
    BlackICE = 9
}

enum VirtualState {
    Connected = 0,
    Disconnecting = 1,
    Flatlined = 2,
    Ghosting = 3,
    DeepDive = 4,
    Tracing = 5,
    Compromised = 6
}

struct VirtualSpace {
    let spaceId: String;
    let spaceName: String;
    let spaceType: VirtualSpaceType;
    let description: String;
    let ownerId: String; // Corporation or player who owns this space
    let accessLevel: AccessLevel;
    let maxUsers: Int32;
    let currentUsers: Int32;
    let connectedRunners: array<String>;
    let securityRating: Int32; // 1-10
    let iceStrength: Int32;
    let dataValue: Int32; // Value of data stored here
    let location: Vector4; // Physical location of servers
    let architecture: String; // Visual style/theme
    let activeNodes: array<String>;
    let traceThreshold: Float; // How long before trace starts
    let alertLevel: Float; // Current security alert
    let isPublic: Bool;
    let accessCodes: array<String>;
    let bannedRunners: array<String>;
    let lastActivity: Float;
    let persistence: Bool; // Does the space persist between sessions
}

struct NetrunnerAvatar {
    let avatarId: String;
    let runnerId: String;
    let runnerName: String;
    let currentSpaceId: String;
    let avatarModel: String; // Visual representation
    let position: Vector4; // Position in cyberspace
    let accessLevel: AccessLevel;
    let state: VirtualState;
    let hackingPower: Int32;
    let stealthRating: Int32;
    let traceResistance: Int32;
    let dataCapacity: Int32;
    let currentData: Int32;
    let installedPrograms: array<String>;
    let activeEffects: array<String>;
    let reputation: Float;
    let sessionTime: Float;
    let lastAction: Float;
    let isVisible: Bool; // Can other runners see them
    let isDeepdiving: Bool;
}

struct CyberNode {
    let nodeId: String;
    let spaceId: String;
    let nodeType: NodeType;
    let nodeName: String;
    let description: String;
    let position: Vector4; // Position in virtual space
    let security: Int32; // Difficulty to hack
    let integrity: Float; // Current health
    let maxIntegrity: Float;
    let isActive: Bool;
    let isCompromised: Bool;
    let ownerId: String;
    let accessRequirements: array<String>;
    let storedData: array<String>;
    let connectedNodes: array<String>;
    let guardianICE: array<String>;
    let lastAccessed: Float;
    let accessLog: array<String>;
    let value: Int32; // Eddies value
    let rarity: String; // "common", "rare", "epic", "legendary"
}

struct ICEProgram {
    let iceId: String;
    let iceName: String;
    let iceType: String; // "defensive", "offensive", "tracker", "black"
    let spaceId: String;
    let targetNodeId: String;
    let strength: Int32;
    let aggressiveness: Int32; // How quickly it attacks
    let detection: Int32; // How good at finding runners
    let persistence: Int32; // How hard to shake off
    let currentTarget: String; // Current runner being pursued
    let isActive: Bool;
    let lastActivity: Float;
    let killCount: Int32; // For Black ICE
    let programCode: String; // AI behavior patterns
}

struct VirtualSession {
    let sessionId: String;
    let spaceId: String;
    let runnerId: String;
    let startTime: Float;
    let endTime: Float;
    let duration: Float;
    let accessMethod: String; // "direct", "proxy", "bounce"
    let nodesAccessed: array<String>;
    let dataExfiltrated: array<String>;
    let iceEncountered: array<String>;
    let tracesEvaded: Int32;
    let alarmsTriggered: Int32;
    let experienceGained: Int32;
    let reputationChange: Float;
    let isSuccessful: Bool;
    let exitReason: String;
    let evidence: array<String>;
}

struct DataPacket {
    let packetId: String;
    let packetName: String;
    let dataType: String; // "corporate_secrets", "financial", "personal", "military"
    let size: Int32; // How much storage it takes
    let value: Int32; // Street value
    let encryption: Int32; // Difficulty to decrypt
    let source: String; // Where it came from
    let destination: String; // Where it's supposed to go
    let expiration: Float; // When it becomes worthless
    let isClassified: Bool;
    let traceMarkers: array<String>; // Tracking data
    let content: String; // Actual data content
    let accessRequirements: array<String>;
}

struct VirtualMarketplace {
    let marketId: String;
    let marketName: String;
    let spaceId: String;
    let marketType: String; // "data", "programs", "ices", "services"
    let vendors: array<String>;
    let activeListings: array<String>;
    let averagePrice: Int32;
    let volume: Int32; // Daily transaction volume
    let reputation: Float;
    let securityLevel: Int32;
    let accessFee: Int32;
    let transactionFee: Float; // Percentage
    let lastUpdated: Float;
}

class VirtualEnvironments {
    private static let instance: ref<VirtualEnvironments>;
    private static let virtualSpaces: array<VirtualSpace>;
    private static let runnerAvatars: array<NetrunnerAvatar>;
    private static let cyberNodes: array<CyberNode>;
    private static let icePrograms: array<ICEProgram>;
    private static let activeSessions: array<VirtualSession>;
    private static let dataPackets: array<DataPacket>;
    private static let virtualMarkets: array<VirtualMarketplace>;
    
    // System configuration
    private static let maxSimultaneousSpaces: Int32 = 50;
    private static let traceTimebase: Float = 300.0; // 5 minutes base trace time
    private static let iceResponseTime: Float = 30.0; // 30 seconds for ICE to respond
    private static let sessionTimeout: Float = 3600.0; // 1 hour max session
    private static let dataDecayRate: Float = 0.1; // Daily data decay
    
    public static func Initialize() -> Void {
        if IsDefined(instance) {
            return;
        }
        instance = new VirtualEnvironments();
        InitializeVirtualSpaces();
        InitializeMarketplaces();
        LogChannel(n"VirtualEnvironments", "Shared virtual netrunning environments initialized");
    }
    
    private static func InitializeVirtualSpaces() -> Void {
        // Arasaka Corporate Network
        let arasakaNet: VirtualSpace;
        arasakaNet.spaceId = "arasaka_corp_net";
        arasakaNet.spaceName = "Arasaka Corporate Network";
        arasakaNet.spaceType = VirtualSpaceType.CorporateNetwork;
        arasakaNet.description = "High-security corporate data fortress containing valuable research and financial data";
        arasakaNet.ownerId = "arasaka_corporation";
        arasakaNet.accessLevel = AccessLevel.Administrator;
        arasakaNet.maxUsers = 10;
        arasakaNet.currentUsers = 0;
        arasakaNet.securityRating = 10;
        arasakaNet.iceStrength = 95;
        arasakaNet.dataValue = 10000000;
        arasakaNet.location = Vector4.Create(100.0, 200.0, 300.0, 1.0);
        arasakaNet.architecture = "japanese_corporate";
        arasakaNet.traceThreshold = 180.0; // 3 minutes
        arasakaNet.alertLevel = 0.0;
        arasakaNet.isPublic = false;
        arasakaNet.persistence = true;
        ArrayPush(virtualSpaces, arasakaNet);
        
        // Night City Public Grid
        let publicGrid: VirtualSpace;
        publicGrid.spaceId = "nc_public_grid";
        publicGrid.spaceName = "Night City Public Grid";
        publicGrid.spaceType = VirtualSpaceType.PublicNet;
        publicGrid.description = "Public network infrastructure accessible to all citizens";
        publicGrid.ownerId = "night_city_gov";
        publicGrid.accessLevel = AccessLevel.Guest;
        publicGrid.maxUsers = 100;
        publicGrid.currentUsers = 0;
        publicGrid.securityRating = 3;
        publicGrid.iceStrength = 25;
        publicGrid.dataValue = 500000;
        publicGrid.location = Vector4.Create(0.0, 0.0, 100.0, 1.0);
        publicGrid.architecture = "urban_grid";
        publicGrid.traceThreshold = 600.0; // 10 minutes
        publicGrid.alertLevel = 0.0;
        publicGrid.isPublic = true;
        publicGrid.persistence = true;
        ArrayPush(virtualSpaces, publicGrid);
        
        // Netrunner Training Ground
        let trainingGround: VirtualSpace;
        trainingGround.spaceId = "netrunner_training";
        trainingGround.spaceName = "Netrunner Training Dojo";
        trainingGround.spaceType = VirtualSpaceType.TrainingGround;
        trainingGround.description = "Safe training environment for practicing netrunning skills";
        trainingGround.ownerId = "system";
        trainingGround.accessLevel = AccessLevel.Guest;
        trainingGround.maxUsers = 50;
        trainingGround.currentUsers = 0;
        trainingGround.securityRating = 5;
        trainingGround.iceStrength = 50;
        trainingGround.dataValue = 0; // No valuable data
        trainingGround.location = Vector4.Create(-100.0, -100.0, 50.0, 1.0);
        trainingGround.architecture = "digital_dojo";
        trainingGround.traceThreshold = 999999.0; // No traces in training
        trainingGround.alertLevel = 0.0;
        trainingGround.isPublic = true;
        trainingGround.persistence = true;
        ArrayPush(virtualSpaces, trainingGround);
        
        // Underground Data Haven
        let dataHaven: VirtualSpace;
        dataHaven.spaceId = "underground_haven";
        dataHaven.spaceName = "Digital Underground";
        dataHaven.spaceType = VirtualSpaceType.DarkWeb;
        dataHaven.description = "Hidden network for illegal data trading and anonymous communication";
        dataHaven.ownerId = "unknown";
        dataHaven.accessLevel = AccessLevel.Authorized;
        dataHaven.maxUsers = 25;
        dataHaven.currentUsers = 0;
        dataHaven.securityRating = 8;
        dataHaven.iceStrength = 70;
        dataHaven.dataValue = 5000000;
        dataHaven.location = Vector4.Create(500.0, -500.0, -50.0, 1.0);
        dataHaven.architecture = "underground";
        dataHaven.traceThreshold = 120.0; // 2 minutes
        dataHaven.alertLevel = 0.0;
        dataHaven.isPublic = false;
        dataHaven.persistence = true;
        ArrayPush(virtualSpaces, dataHaven);
    }
    
    private static func InitializeMarketplaces() -> Void {
        // Data Bazaar
        let dataBazaar: VirtualMarketplace;
        dataBazaar.marketId = "data_bazaar";
        dataBazaar.marketName = "The Data Bazaar";
        dataBazaar.spaceId = "underground_haven";
        dataBazaar.marketType = "data";
        dataBazaar.averagePrice = 50000;
        dataBazaar.volume = 1000000;
        dataBazaar.reputation = 4.2;
        dataBazaar.securityLevel = 7;
        dataBazaar.accessFee = 5000;
        dataBazaar.transactionFee = 0.05; // 5%
        dataBazaar.lastUpdated = GetGameTime();
        ArrayPush(virtualMarkets, dataBazaar);
        
        // Program Exchange
        let programExchange: VirtualMarketplace;
        programExchange.marketId = "program_exchange";
        programExchange.marketName = "NetRunner's Exchange";
        programExchange.spaceId = "nc_public_grid";
        programExchange.marketType = "programs";
        programExchange.averagePrice = 25000;
        programExchange.volume = 500000;
        programExchange.reputation = 3.8;
        programExchange.securityLevel = 4;
        programExchange.accessFee = 1000;
        programExchange.transactionFee = 0.03; // 3%
        programExchange.lastUpdated = GetGameTime();
        ArrayPush(virtualMarkets, programExchange);
    }
    
    // Connection and session management
    public static func ConnectToVirtualSpace(runnerId: String, spaceId: String, accessMethod: String, credentials: array<String>) -> String {
        let space = GetVirtualSpace(spaceId);
        if !IsDefined(space) {
            return "";
        }
        
        if !CanAccessSpace(runnerId, space, credentials) {
            return "";
        }
        
        if space.currentUsers >= space.maxUsers {
            return "";
        }
        
        let sessionId = runnerId + "_" + spaceId + "_" + ToString(GetGameTime());
        
        // Create avatar if it doesn't exist
        if !HasAvatar(runnerId) {
            CreateRunnerAvatar(runnerId);
        }
        
        // Update avatar state
        let avatarIndex = GetAvatarIndex(runnerId);
        if avatarIndex != -1 {
            runnerAvatars[avatarIndex].currentSpaceId = spaceId;
            runnerAvatars[avatarIndex].state = VirtualState.Connected;
            runnerAvatars[avatarIndex].sessionTime = GetGameTime();
            runnerAvatars[avatarIndex].position = GetSpawnPosition(space);
        }
        
        // Create session
        let session: VirtualSession;
        session.sessionId = sessionId;
        session.spaceId = spaceId;
        session.runnerId = runnerId;
        session.startTime = GetGameTime();
        session.accessMethod = accessMethod;
        session.isSuccessful = false;
        
        ArrayPush(activeSessions, session);
        
        // Update space
        let spaceIndex = GetSpaceIndex(spaceId);
        if spaceIndex != -1 {
            ArrayPush(virtualSpaces[spaceIndex].connectedRunners, runnerId);
            virtualSpaces[spaceIndex].currentUsers += 1;
            virtualSpaces[spaceIndex].lastActivity = GetGameTime();
        }
        
        // Notify other connected runners
        let connectData = "runner:" + PlayerSystem.GetPlayerName(runnerId) + ",method:" + accessMethod;
        BroadcastToSpace(spaceId, "runner_connected", connectData, runnerId);
        
        // Send space data to runner
        let spaceData = JsonStringify(space);
        NetworkingSystem.SendToPlayer(runnerId, "virtual_space_entered", spaceData);
        
        // Start ICE monitoring if applicable
        if space.securityRating > 5 {
            InitiateICEMonitoring(spaceId, runnerId);
        }
        
        LogChannel(n"VirtualEnvironments", StrCat("Runner connected to virtual space: ", runnerId, " -> ", spaceId));
        return sessionId;
    }
    
    public static func DisconnectFromVirtualSpace(runnerId: String, reason: String) -> Void {
        let avatar = GetRunnerAvatar(runnerId);
        if !IsDefined(avatar) {
            return;
        }
        
        let spaceId = avatar.currentSpaceId;
        let sessionIndex = GetActiveSessionIndex(runnerId, spaceId);
        
        if sessionIndex != -1 {
            // Complete session
            let session = activeSessions[sessionIndex];
            session.endTime = GetGameTime();
            session.duration = session.endTime - session.startTime;
            session.exitReason = reason;
            
            // Move to history
            ArrayRemove(activeSessions, session);
        }
        
        // Update avatar
        let avatarIndex = GetAvatarIndex(runnerId);
        if avatarIndex != -1 {
            runnerAvatars[avatarIndex].currentSpaceId = "";
            runnerAvatars[avatarIndex].state = VirtualState.Disconnecting;
        }
        
        // Update space
        let spaceIndex = GetSpaceIndex(spaceId);
        if spaceIndex != -1 {
            ArrayRemove(virtualSpaces[spaceIndex].connectedRunners, runnerId);
            virtualSpaces[spaceIndex].currentUsers -= 1;
        }
        
        // Notify other runners
        let disconnectData = "runner:" + PlayerSystem.GetPlayerName(runnerId) + ",reason:" + reason;
        BroadcastToSpace(spaceId, "runner_disconnected", disconnectData, runnerId);
        
        LogChannel(n"VirtualEnvironments", StrCat("Runner disconnected from virtual space: ", runnerId, " Reason: ", reason));
    }
    
    // Node interaction and hacking
    public static func AccessNode(runnerId: String, nodeId: String, hackingProgram: String) -> Bool {
        let node = GetCyberNode(nodeId);
        if !IsDefined(node) {
            return false;
        }
        
        let avatar = GetRunnerAvatar(runnerId);
        if !IsDefined(avatar) || !Equals(avatar.currentSpaceId, node.spaceId) {
            return false;
        }
        
        // Check if runner can access this node
        if !CanAccessNode(avatar, node) {
            return false;
        }
        
        // Calculate success chance
        let successChance = CalculateHackingSuccess(avatar, node, hackingProgram);
        let success = RandF() <= successChance;
        
        if success {
            // Grant access
            node.isCompromised = true;
            node.lastAccessed = GetGameTime();
            
            let accessEntry = ToString(GetGameTime()) + ":" + runnerId + ":" + hackingProgram;
            ArrayPush(node.accessLog, accessEntry);
            
            // Update session data
            UpdateSessionProgress(runnerId, node.spaceId, nodeId, true);
            
            // Trigger ICE response if security is high
            if node.security >= 7 {
                TriggerICEResponse(node.spaceId, runnerId, nodeId);
            }
            
            // Notify runner
            let accessData = "node:" + node.nodeName + ",type:" + ToString(Cast<Int32>(node.nodeType)) + ",value:" + ToString(node.value);
            NetworkingSystem.SendToPlayer(runnerId, "node_accessed", accessData);
            
            return true;
        } else {
            // Failed attempt
            UpdateSessionProgress(runnerId, node.spaceId, nodeId, false);
            
            // Increase alert level
            IncreaseAlertLevel(node.spaceId, 0.1);
            
            // Notify runner of failure
            NetworkingSystem.SendToPlayer(runnerId, "node_access_failed", "");
            
            return false;
        }
    }
    
    public static func ExfiltrateData(runnerId: String, nodeId: String, dataPacketIds: array<String>) -> Bool {
        let node = GetCyberNode(nodeId);
        if !IsDefined(node) || !node.isCompromised {
            return false;
        }
        
        let avatar = GetRunnerAvatar(runnerId);
        if !IsDefined(avatar) {
            return false;
        }
        
        let totalSize = 0;
        for packetId in dataPacketIds {
            let packet = GetDataPacket(packetId);
            if IsDefined(packet) {
                totalSize += packet.size;
            }
        }
        
        // Check if runner has enough data capacity
        if avatar.currentData + totalSize > avatar.dataCapacity {
            return false;
        }
        
        // Transfer data
        let avatarIndex = GetAvatarIndex(runnerId);
        if avatarIndex != -1 {
            runnerAvatars[avatarIndex].currentData += totalSize;
        }
        
        // Remove data from node
        for packetId in dataPacketIds {
            ArrayRemove(node.storedData, packetId);
        }
        
        // Update session
        let sessionIndex = GetActiveSessionIndex(runnerId, node.spaceId);
        if sessionIndex != -1 {
            ArrayConcatenate(activeSessions[sessionIndex].dataExfiltrated, dataPacketIds);
        }
        
        // Increase alert significantly
        IncreaseAlertLevel(node.spaceId, 0.3);
        
        // Notify runner
        let exfilData = "packets:" + ToString(ArraySize(dataPacketIds)) + ",size:" + ToString(totalSize);
        NetworkingSystem.SendToPlayer(runnerId, "data_exfiltrated", exfilData);
        
        LogChannel(n"VirtualEnvironments", StrCat("Data exfiltrated: ", runnerId, " from ", nodeId));
        return true;
    }
    
    // ICE and security systems
    public static func TriggerICEResponse(spaceId: String, targetRunnerId: String, sourceNodeId: String) -> Void {
        let space = GetVirtualSpace(spaceId);
        if !IsDefined(space) {
            return;
        }
        
        // Find available ICE programs
        let availableICE: array<ICEProgram>;
        for ice in icePrograms {
            if Equals(ice.spaceId, spaceId) && ice.isActive && Equals(ice.currentTarget, "") {
                ArrayPush(availableICE, ice);
            }
        }
        
        if ArraySize(availableICE) == 0 {
            return;
        }
        
        // Select appropriate ICE based on threat level
        let selectedICE = SelectICEForThreat(availableICE, space.alertLevel);
        
        // Assign ICE to target
        let iceIndex = GetICEIndex(selectedICE.iceId);
        if iceIndex != -1 {
            icePrograms[iceIndex].currentTarget = targetRunnerId;
            icePrograms[iceIndex].lastActivity = GetGameTime();
        }
        
        // Notify target
        let iceData = "ice_type:" + selectedICE.iceType + ",strength:" + ToString(selectedICE.strength);
        NetworkingSystem.SendToPlayer(targetRunnerId, "ice_pursuing", iceData);
        
        // Start ICE pursuit
        StartICEPursuit(selectedICE.iceId, targetRunnerId);
        
        LogChannel(n"VirtualEnvironments", StrCat("ICE triggered: ", selectedICE.iceId, " targeting ", targetRunnerId));
    }
    
    public static func EvadeICE(runnerId: String, iceId: String, evasionProgram: String) -> Bool {
        let ice = GetICEProgram(iceId);
        if !IsDefined(ice) || !Equals(ice.currentTarget, runnerId) {
            return false;
        }
        
        let avatar = GetRunnerAvatar(runnerId);
        if !IsDefined(avatar) {
            return false;
        }
        
        // Calculate evasion success
        let evasionChance = CalculateEvasionChance(avatar, ice, evasionProgram);
        let success = RandF() <= evasionChance;
        
        if success {
            // Successfully evaded
            let iceIndex = GetICEIndex(iceId);
            if iceIndex != -1 {
                icePrograms[iceIndex].currentTarget = "";
            }
            
            // Update session stats
            let sessionIndex = GetActiveSessionIndex(runnerId, avatar.currentSpaceId);
            if sessionIndex != -1 {
                activeSessions[sessionIndex].tracesEvaded += 1;
            }
            
            NetworkingSystem.SendToPlayer(runnerId, "ice_evaded", "");
            return true;
        } else {
            // Evasion failed, ICE continues pursuit
            NetworkingSystem.SendToPlayer(runnerId, "ice_evasion_failed", "");
            
            // Potentially cause damage or effects
            ApplyICEEffects(runnerId, ice);
            
            return false;
        }
    }
    
    // Collaborative netrunning
    public static func CreateNetrunningTeam(leaderId: String, teamName: String, members: array<String>, targetSpaceId: String) -> String {
        let teamId = "team_" + leaderId + "_" + ToString(GetGameTime());
        
        // Verify all members can access the target space
        for memberId in members {
            if !CanParticipateInTeam(memberId, targetSpaceId) {
                return "";
            }
        }
        
        // Create team session
        let teamData = "team_id:" + teamId + ",leader:" + leaderId + ",target:" + targetSpaceId + ",members:" + ToString(ArraySize(members));
        
        // Notify all team members
        for memberId in members {
            NetworkingSystem.SendToPlayer(memberId, "netrunning_team_created", teamData);
        }
        
        LogChannel(n"VirtualEnvironments", StrCat("Netrunning team created: ", teamId));
        return teamId;
    }
    
    public static func ShareRunnerData(senderId: String, receiverId: String, dataPacketIds: array<String>) -> Bool {
        let senderAvatar = GetRunnerAvatar(senderId);
        let receiverAvatar = GetRunnerAvatar(receiverId);
        
        if !IsDefined(senderAvatar) || !IsDefined(receiverAvatar) {
            return false;
        }
        
        // Must be in same virtual space
        if !Equals(senderAvatar.currentSpaceId, receiverAvatar.currentSpaceId) {
            return false;
        }
        
        let totalSize = 0;
        for packetId in dataPacketIds {
            let packet = GetDataPacket(packetId);
            if IsDefined(packet) {
                totalSize += packet.size;
            }
        }
        
        // Check receiver capacity
        if receiverAvatar.currentData + totalSize > receiverAvatar.dataCapacity {
            return false;
        }
        
        // Transfer data
        let senderIndex = GetAvatarIndex(senderId);
        let receiverIndex = GetAvatarIndex(receiverId);
        
        if senderIndex != -1 && receiverIndex != -1 {
            runnerAvatars[senderIndex].currentData -= totalSize;
            runnerAvatars[receiverIndex].currentData += totalSize;
        }
        
        // Notify both parties
        let transferData = "size:" + ToString(totalSize) + ",packets:" + ToString(ArraySize(dataPacketIds));
        NetworkingSystem.SendToPlayer(senderId, "data_sent", transferData);
        NetworkingSystem.SendToPlayer(receiverId, "data_received", transferData);
        
        return true;
    }
    
    // Market and trading functions
    public static func ListDataForSale(sellerId: String, dataPacketId: String, price: Int32, marketId: String) -> String {
        let packet = GetDataPacket(dataPacketId);
        if !IsDefined(packet) {
            return "";
        }
        
        let market = GetVirtualMarket(marketId);
        if !IsDefined(market) {
            return "";
        }
        
        // Verify seller has access to market
        if !HasMarketAccess(sellerId, marketId) {
            return "";
        }
        
        let listingId = marketId + "_" + sellerId + "_" + ToString(GetGameTime());
        
        // Create market listing
        let listingData = "packet_id:" + dataPacketId + ",price:" + ToString(price) + ",seller:" + sellerId;
        
        // Add to market
        let marketIndex = GetMarketIndex(marketId);
        if marketIndex != -1 {
            ArrayPush(virtualMarkets[marketIndex].activeListings, listingId);
            virtualMarkets[marketIndex].lastUpdated = GetGameTime();
        }
        
        // Notify market participants
        BroadcastToMarket(marketId, "new_listing", listingData);
        
        LogChannel(n"VirtualEnvironments", StrCat("Data listed for sale: ", listingId));
        return listingId;
    }
    
    // Utility functions
    private static func GetVirtualSpace(spaceId: String) -> VirtualSpace {
        for space in virtualSpaces {
            if Equals(space.spaceId, spaceId) {
                return space;
            }
        }
        
        let emptySpace: VirtualSpace;
        return emptySpace;
    }
    
    private static func GetRunnerAvatar(runnerId: String) -> NetrunnerAvatar {
        for avatar in runnerAvatars {
            if Equals(avatar.runnerId, runnerId) {
                return avatar;
            }
        }
        
        let emptyAvatar: NetrunnerAvatar;
        return emptyAvatar;
    }
    
    private static func GetCyberNode(nodeId: String) -> CyberNode {
        for node in cyberNodes {
            if Equals(node.nodeId, nodeId) {
                return node;
            }
        }
        
        let emptyNode: CyberNode;
        return emptyNode;
    }
    
    private static func CanAccessSpace(runnerId: String, space: VirtualSpace, credentials: array<String>) -> Bool {
        // Public spaces allow anyone
        if space.isPublic && space.accessLevel <= AccessLevel.Guest {
            return true;
        }
        
        // Check if banned
        if ArrayContains(space.bannedRunners, runnerId) {
            return false;
        }
        
        // Check credentials
        for credential in credentials {
            if ArrayContains(space.accessCodes, credential) {
                return true;
            }
        }
        
        // Check player skills/reputation
        return MeetsAccessRequirements(runnerId, space);
    }
    
    private static func CreateRunnerAvatar(runnerId: String) -> Void {
        let avatar: NetrunnerAvatar;
        avatar.avatarId = "avatar_" + runnerId;
        avatar.runnerId = runnerId;
        avatar.runnerName = PlayerSystem.GetPlayerName(runnerId);
        avatar.currentSpaceId = "";
        avatar.avatarModel = GetDefaultAvatarModel();
        avatar.position = Vector4.Create(0.0, 0.0, 0.0, 1.0);
        avatar.accessLevel = AccessLevel.Guest;
        avatar.state = VirtualState.Disconnecting;
        avatar.hackingPower = GetPlayerHackingSkill(runnerId);
        avatar.stealthRating = GetPlayerStealthSkill(runnerId);
        avatar.traceResistance = GetPlayerResistance(runnerId);
        avatar.dataCapacity = GetPlayerDataCapacity(runnerId);
        avatar.currentData = 0;
        avatar.installedPrograms = GetPlayerPrograms(runnerId);
        avatar.reputation = 100.0;
        avatar.sessionTime = 0.0;
        avatar.lastAction = GetGameTime();
        avatar.isVisible = true;
        avatar.isDeepdiving = false;
        
        ArrayPush(runnerAvatars, avatar);
    }
    
    public static func GetConnectedSpaces() -> array<VirtualSpace> {
        let connectedSpaces: array<VirtualSpace>;
        
        for space in virtualSpaces {
            if space.currentUsers > 0 {
                ArrayPush(connectedSpaces, space);
            }
        }
        
        return connectedSpaces;
    }
    
    public static func GetSpaceRunners(spaceId: String) -> array<NetrunnerAvatar> {
        let spaceRunners: array<NetrunnerAvatar>;
        
        for avatar in runnerAvatars {
            if Equals(avatar.currentSpaceId, spaceId) && avatar.state == VirtualState.Connected {
                ArrayPush(spaceRunners, avatar);
            }
        }
        
        return spaceRunners;
    }
    
    public static func GetPlayerNetrunningHistory(runnerId: String) -> array<VirtualSession> {
        let history: array<VirtualSession>;
        
        // This would query completed sessions from a history database
        // For now, return empty array as placeholder
        
        return history;
    }
}