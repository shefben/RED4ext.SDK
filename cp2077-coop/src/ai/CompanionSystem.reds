// AI Companion System for Cyberpunk 2077 Multiplayer
// Shared AI allies with unique personalities and abilities for cooperative play

module CompanionSystem

enum CompanionType {
    Mercenary = 0,
    Netrunner = 1,
    Techie = 2,
    Medic = 3,
    Fixer = 4,
    Bodyguard = 5,
    Driver = 6,
    Infiltrator = 7,
    Support = 8,
    Specialist = 9
}

enum PersonalityTrait {
    Aggressive = 0,
    Cautious = 1,
    Loyal = 2,
    Independent = 3,
    Analytical = 4,
    Empathetic = 5,
    Pragmatic = 6,
    Idealistic = 7,
    Sarcastic = 8,
    Optimistic = 9
}

enum CompanionRarity {
    Common = 0,
    Uncommon = 1,
    Rare = 2,
    Epic = 3,
    Legendary = 4,
    Unique = 5
}

enum CompanionStatus {
    Available = 0,
    Deployed = 1,
    OnMission = 2,
    Injured = 3,
    InTraining = 4,
    Unavailable = 5,
    KIA = 6
}

enum ShareType {
    Temporary = 0,
    Permanent = 1,
    Mission = 2,
    Rental = 3,
    Gift = 4
}

struct AICompanion {
    let companionId: String;
    let companionName: String;
    let displayName: String;
    let companionType: CompanionType;
    let rarity: CompanionRarity;
    let ownerId: String;
    let level: Int32;
    let experience: Int32;
    let nextLevelXP: Int32;
    let status: CompanionStatus;
    let personality: array<PersonalityTrait>;
    let backstory: String;
    let appearance: String; // Visual customization data
    let voiceSet: String;
    let combatStyle: String;
    let specializations: array<String>;
    let skills: array<Int32>; // Skill levels
    let attributes: array<Int32>; // Base attributes
    let equipment: array<String>; // Equipped gear
    let cyberware: array<String>; // Installed cyberware
    let relationships: array<String>; // Relationships with other companions/players
    let morale: Float; // 0.0-1.0
    let loyalty: Float; // 0.0-1.0
    let fatigue: Float; // 0.0-1.0
    let injuries: array<String>; // Current injuries
    let traits: array<String>; // Special traits and abilities
    let unlockDate: Float;
    let totalDeployments: Int32;
    let successfulMissions: Int32;
    let killCount: Int32;
    let lastDeployment: Float;
    let maintenanceCost: Int32; // Daily upkeep
    let marketValue: Int32; // Current trading value
}

struct CompanionShare {
    let shareId: String;
    let companionId: String;
    let ownerId: String;
    let sharedWithId: String;
    let shareType: ShareType;
    let startTime: Float;
    let duration: Float; // 0 = permanent
    let remainingTime: Float;
    let cost: Int32; // Cost to share/rent
    let conditions: array<String>; // Special conditions
    let permissions: array<String>; // What the borrower can do
    let isActive: Bool;
    let returnCondition: String; // When to return
}

struct CompanionDeployment {
    let deploymentId: String;
    let companionId: String;
    let deployerId: String;
    let missionId: String;
    let deploymentType: String; // "solo", "squad", "support"
    let startTime: Float;
    let endTime: Float;
    let location: Vector4;
    let objectives: array<String>;
    let completedObjectives: array<String>;
    let experienceGained: Int32;
    let injuriesSustained: array<String>;
    let performanceRating: Float;
    let lootGathered: array<String>;
    let isSuccessful: Bool;
}

struct CompanionAI {
    let companionId: String;
    let aiPersonality: String; // AI behavior profile
    let decisionMaking: String; // How they make choices
    let combatTactics: String; // Preferred combat approach
    let socialBehavior: String; // How they interact
    let learningRate: Float; // How quickly they adapt
    let adaptability: Float; // How well they handle new situations
    let initiative: Float; // How proactive they are
    let communicationStyle: String;
    let preferredRoles: array<String>;
    let dislikes: array<String>; // Things that lower morale
    let likes: array<String>; // Things that boost morale
    let memoryBank: array<String>; // Remembered events and lessons
}

struct CompanionInteraction {
    let interactionId: String;
    let companionId: String;
    let playerId: String;
    let interactionType: String; // "conversation", "training", "customization"
    let timestamp: Float;
    let location: Vector4;
    let content: String;
    let choices: array<String>;
    let playerResponse: String;
    let companionReaction: String;
    let relationshipChange: Float;
    let moraleChange: Float;
    let outcomes: array<String>;
}

struct CompanionMarket {
    let listingId: String;
    let companionId: String;
    let sellerId: String;
    let listingType: String; // "sale", "rental", "sharing"
    let price: Int32;
    let rentalRate: Int32; // Per day
    let description: String;
    let terms: array<String>;
    let viewCount: Int32;
    let inquiries: Int32;
    let listingDate: Float;
    let expirationDate: Float;
    let isFeatured: Bool;
    let category: CompanionType;
    let minLevel: Int32; // Buyer level requirement
}

class CompanionSystem {
    private static let instance: ref<CompanionSystem>;
    private static let registeredCompanions: array<AICompanion>;
    private static let companionShares: array<CompanionShare>;
    private static let activeDeployments: array<CompanionDeployment>;
    private static let companionAI: array<CompanionAI>;
    private static let recentInteractions: array<CompanionInteraction>;
    private static let marketListings: array<CompanionMarket>;
    
    // System configuration
    private static let maxCompanionsPerPlayer: Int32 = 5;
    private static let maxSharedCompanions: Int32 = 3;
    private static let deploymentCooldown: Float = 3600.0; // 1 hour between deployments
    private static let baseMaintenance: Int32 = 500; // Base daily cost
    private static let moraleDecayRate: Float = 0.05; // Daily morale decay
    private static let fatigueBuildupRate: Float = 0.1; // Per deployment hour
    
    public static func Initialize() -> Void {
        if IsDefined(instance) {
            return;
        }
        instance = new CompanionSystem();
        LogChannel(n"CompanionSystem", "AI companion system initialized");
    }
    
    // Companion acquisition and management
    public static func AcquireCompanion(playerId: String, companionType: CompanionType, acquisitionMethod: String) -> String {
        if GetPlayerCompanionCount(playerId) >= maxCompanionsPerPlayer {
            return "";
        }
        
        let companionId = "companion_" + playerId + "_" + ToString(GetGameTime());
        
        let companion: AICompanion;
        companion.companionId = companionId;
        companion.companionName = GenerateCompanionName(companionType);
        companion.displayName = companion.companionName;
        companion.companionType = companionType;
        companion.rarity = DetermineCompanionRarity(acquisitionMethod);
        companion.ownerId = playerId;
        companion.level = 1;
        companion.experience = 0;
        companion.nextLevelXP = CalculateNextLevelXP(1);
        companion.status = CompanionStatus.Available;
        companion.personality = GeneratePersonality();
        companion.backstory = GenerateBackstory(companionType);
        companion.appearance = GenerateAppearance(companionType);
        companion.voiceSet = SelectVoiceSet(companionType);
        companion.combatStyle = DetermineCombatStyle(companionType);
        companion.specializations = GenerateSpecializations(companionType);
        companion.skills = GenerateInitialSkills(companionType, companion.rarity);
        companion.attributes = GenerateInitialAttributes(companionType);
        companion.morale = 0.8; // Good starting morale
        companion.loyalty = 0.5; // Neutral loyalty
        companion.fatigue = 0.0;
        companion.unlockDate = GetGameTime();
        companion.totalDeployments = 0;
        companion.successfulMissions = 0;
        companion.killCount = 0;
        companion.lastDeployment = 0.0;
        companion.maintenanceCost = CalculateMaintenanceCost(companion.rarity);
        companion.marketValue = CalculateMarketValue(companion);
        
        ArrayPush(registeredCompanions, companion);
        
        // Create AI profile
        CreateCompanionAI(companion);
        
        let companionData = JsonStringify(companion);
        NetworkingSystem.SendToPlayer(playerId, "companion_acquired", companionData);
        
        LogChannel(n"CompanionSystem", StrCat("Companion acquired: ", companionId, " by ", playerId));
        return companionId;
    }
    
    public static func ShareCompanion(ownerId: String, companionId: String, targetId: String, shareType: ShareType, duration: Float, cost: Int32) -> String {
        let companion = GetCompanion(companionId);
        if !IsDefined(companion) || !Equals(companion.ownerId, ownerId) {
            return "";
        }
        
        if companion.status != CompanionStatus.Available {
            return "";
        }
        
        if GetSharedCompanionCount(targetId) >= maxSharedCompanions {
            return "";
        }
        
        let shareId = companionId + "_share_" + ToString(GetGameTime());
        
        let share: CompanionShare;
        share.shareId = shareId;
        share.companionId = companionId;
        share.ownerId = ownerId;
        share.sharedWithId = targetId;
        share.shareType = shareType;
        share.startTime = GetGameTime();
        share.duration = duration;
        share.remainingTime = duration;
        share.cost = cost;
        share.isActive = true;
        
        // Set permissions based on share type
        if shareType == ShareType.Temporary || shareType == ShareType.Rental {
            ArrayPush(share.permissions, "deploy");
            ArrayPush(share.permissions, "train");
        } else if shareType == ShareType.Mission {
            ArrayPush(share.permissions, "deploy");
        } else if shareType == ShareType.Permanent || shareType == ShareType.Gift {
            ArrayPush(share.permissions, "full_control");
            
            // Transfer ownership for permanent shares/gifts
            if shareType == ShareType.Permanent || shareType == ShareType.Gift {
                let companionIndex = GetCompanionIndex(companionId);
                if companionIndex != -1 {
                    registeredCompanions[companionIndex].ownerId = targetId;
                }
            }
        }
        
        ArrayPush(companionShares, share);
        
        // Handle payment
        if cost > 0 {
            EconomySystem.TransferFunds(targetId, ownerId, cost);
        }
        
        // Notify both parties
        let shareData = "companion:" + companion.displayName + ",type:" + ToString(Cast<Int32>(shareType)) + ",duration:" + ToString(duration);
        NetworkingSystem.SendToPlayer(ownerId, "companion_shared", shareData);
        NetworkingSystem.SendToPlayer(targetId, "companion_received", shareData);
        
        LogChannel(n"CompanionSystem", StrCat("Companion shared: ", companionId, " from ", ownerId, " to ", targetId));
        return shareId;
    }
    
    public static func DeployCompanion(playerId: String, companionId: String, missionId: String, deploymentType: String, objectives: array<String>) -> String {
        let companion = GetCompanion(companionId);
        if !IsDefined(companion) {
            return "";
        }
        
        if !CanDeployCompanion(playerId, companion) {
            return "";
        }
        
        let deploymentId = companionId + "_deploy_" + ToString(GetGameTime());
        
        let deployment: CompanionDeployment;
        deployment.deploymentId = deploymentId;
        deployment.companionId = companionId;
        deployment.deployerId = playerId;
        deployment.missionId = missionId;
        deployment.deploymentType = deploymentType;
        deployment.startTime = GetGameTime();
        deployment.location = PlayerSystem.GetPlayerLocation(playerId);
        deployment.objectives = objectives;
        deployment.experienceGained = 0;
        deployment.performanceRating = 0.0;
        deployment.isSuccessful = false;
        
        ArrayPush(activeDeployments, deployment);
        
        // Update companion status
        let companionIndex = GetCompanionIndex(companionId);
        if companionIndex != -1 {
            registeredCompanions[companionIndex].status = CompanionStatus.Deployed;
            registeredCompanions[companionIndex].totalDeployments += 1;
            registeredCompanions[companionIndex].lastDeployment = GetGameTime();
        }
        
        // Spawn companion in game world
        SpawnCompanionInWorld(companionId, playerId, deployment.location);
        
        let deployData = "companion:" + companion.displayName + ",mission:" + missionId;
        NetworkingSystem.SendToPlayer(playerId, "companion_deployed", deployData);
        
        LogChannel(n"CompanionSystem", StrCat("Companion deployed: ", companionId, " by ", playerId));
        return deploymentId;
    }
    
    public static func RecallCompanion(playerId: String, companionId: String, reason: String) -> Bool {
        let companion = GetCompanion(companionId);
        if !IsDefined(companion) {
            return false;
        }
        
        if !CanRecallCompanion(playerId, companion) {
            return false;
        }
        
        // Find active deployment
        let deploymentIndex = GetActiveDeploymentIndex(companionId);
        if deploymentIndex != -1 {
            let deployment = activeDeployments[deploymentIndex];
            deployment.endTime = GetGameTime();
            deployment.isSuccessful = Equals(reason, "mission_complete");
            
            // Calculate performance and rewards
            CalculateDeploymentResults(deployment);
            
            // Remove from active deployments
            ArrayRemove(activeDeployments, deployment);
        }
        
        // Update companion status
        let companionIndex = GetCompanionIndex(companionId);
        if companionIndex != -1 {
            registeredCompanions[companionIndex].status = CompanionStatus.Available;
            
            // Add fatigue
            registeredCompanions[companionIndex].fatigue += fatigueBuildupRate * 2.0; // 2 hours worth
            registeredCompanions[companionIndex].fatigue = MinF(registeredCompanions[companionIndex].fatigue, 1.0);
        }
        
        // Despawn from world
        DespawnCompanionFromWorld(companionId);
        
        let recallData = "companion:" + companion.displayName + ",reason:" + reason;
        NetworkingSystem.SendToPlayer(playerId, "companion_recalled", recallData);
        
        return true;
    }
    
    // Training and development
    public static func TrainCompanion(playerId: String, companionId: String, trainingType: String, duration: Float, cost: Int32) -> Bool {
        let companion = GetCompanion(companionId);
        if !IsDefined(companion) {
            return false;
        }
        
        if !CanTrainCompanion(playerId, companion) {
            return false;
        }
        
        if !EconomySystem.HasFunds(playerId, cost) {
            return false;
        }
        
        // Set companion to training status
        let companionIndex = GetCompanionIndex(companionId);
        if companionIndex != -1 {
            registeredCompanions[companionIndex].status = CompanionStatus.InTraining;
        }
        
        // Schedule training completion
        ScheduleTrainingCompletion(companionId, playerId, trainingType, duration);
        
        // Charge training cost
        EconomySystem.ChargeFunds(playerId, cost);
        
        let trainData = "companion:" + companion.displayName + ",type:" + trainingType + ",duration:" + ToString(duration);
        NetworkingSystem.SendToPlayer(playerId, "companion_training_started", trainData);
        
        return true;
    }
    
    public static func CompleteTraining(companionId: String, trainingType: String) -> Void {
        let companionIndex = GetCompanionIndex(companionId);
        if companionIndex == -1 {
            return;
        }
        
        let companion = registeredCompanions[companionIndex];
        
        // Apply training benefits
        ApplyTrainingBenefits(companion, trainingType);
        
        // Update status
        companion.status = CompanionStatus.Available;
        companion.morale += 0.1; // Training boosts morale
        
        registeredCompanions[companionIndex] = companion;
        
        let completeData = "companion:" + companion.displayName + ",type:" + trainingType;
        NetworkingSystem.SendToPlayer(companion.ownerId, "companion_training_completed", completeData);
    }
    
    // Interaction and relationship building
    public static func InteractWithCompanion(playerId: String, companionId: String, interactionType: String, content: String) -> String {
        let companion = GetCompanion(companionId);
        if !IsDefined(companion) {
            return "";
        }
        
        let interactionId = companionId + "_interact_" + ToString(GetGameTime());
        
        let interaction: CompanionInteraction;
        interaction.interactionId = interactionId;
        interaction.companionId = companionId;
        interaction.playerId = playerId;
        interaction.interactionType = interactionType;
        interaction.timestamp = GetGameTime();
        interaction.location = PlayerSystem.GetPlayerLocation(playerId);
        interaction.content = content;
        
        // Generate companion response based on AI and personality
        interaction.companionReaction = GenerateCompanionResponse(companion, interactionType, content);
        
        // Calculate relationship and morale changes
        let relationshipChange = CalculateRelationshipChange(companion, interactionType, content);
        let moraleChange = CalculateMoraleChange(companion, interactionType, content);
        
        interaction.relationshipChange = relationshipChange;
        interaction.moraleChange = moraleChange;
        
        // Apply changes
        let companionIndex = GetCompanionIndex(companionId);
        if companionIndex != -1 {
            // Update loyalty (based on relationship)
            registeredCompanions[companionIndex].loyalty += relationshipChange;
            registeredCompanions[companionIndex].loyalty = ClampF(registeredCompanions[companionIndex].loyalty, 0.0, 1.0);
            
            // Update morale
            registeredCompanions[companionIndex].morale += moraleChange;
            registeredCompanions[companionIndex].morale = ClampF(registeredCompanions[companionIndex].morale, 0.0, 1.0);
        }
        
        ArrayPush(recentInteractions, interaction);
        
        // Send response to player
        let responseData = "response:" + interaction.companionReaction + ",loyalty:" + ToString(companion.loyalty) + ",morale:" + ToString(companion.morale);
        NetworkingSystem.SendToPlayer(playerId, "companion_interaction_response", responseData);
        
        return interactionId;
    }
    
    // Market and trading
    public static func ListCompanionForSale(ownerId: String, companionId: String, price: Int32, terms: array<String>) -> String {
        let companion = GetCompanion(companionId);
        if !IsDefined(companion) || !Equals(companion.ownerId, ownerId) {
            return "";
        }
        
        if companion.status != CompanionStatus.Available {
            return "";
        }
        
        let listingId = companionId + "_listing_" + ToString(GetGameTime());
        
        let listing: CompanionMarket;
        listing.listingId = listingId;
        listing.companionId = companionId;
        listing.sellerId = ownerId;
        listing.listingType = "sale";
        listing.price = price;
        listing.description = GenerateListingDescription(companion);
        listing.terms = terms;
        listing.viewCount = 0;
        listing.inquiries = 0;
        listing.listingDate = GetGameTime();
        listing.expirationDate = GetGameTime() + 604800.0; // 1 week
        listing.isFeatured = false;
        listing.category = companion.companionType;
        listing.minLevel = CalculateMinBuyerLevel(companion);
        
        ArrayPush(marketListings, listing);
        
        // Mark companion as unavailable while listed
        let companionIndex = GetCompanionIndex(companionId);
        if companionIndex != -1 {
            registeredCompanions[companionIndex].status = CompanionStatus.Unavailable;
        }
        
        let listData = "companion:" + companion.displayName + ",price:" + ToString(price);
        NetworkingSystem.SendToPlayer(ownerId, "companion_listed", listData);
        NetworkingSystem.BroadcastMessage("companion_market_new_listing", listData);
        
        return listingId;
    }
    
    public static func PurchaseCompanion(buyerId: String, listingId: String) -> Bool {
        let listingIndex = GetMarketListingIndex(listingId);
        if listingIndex == -1 {
            return false;
        }
        
        let listing = marketListings[listingIndex];
        
        if !EconomySystem.HasFunds(buyerId, listing.price) {
            return false;
        }
        
        if GetPlayerCompanionCount(buyerId) >= maxCompanionsPerPlayer {
            return false;
        }
        
        // Transfer ownership
        let companionIndex = GetCompanionIndex(listing.companionId);
        if companionIndex != -1 {
            registeredCompanions[companionIndex].ownerId = buyerId;
            registeredCompanions[companionIndex].status = CompanionStatus.Available;
            registeredCompanions[companionIndex].loyalty = 0.3; // Lower loyalty with new owner
        }
        
        // Process payment
        EconomySystem.TransferFunds(buyerId, listing.sellerId, listing.price);
        
        // Remove listing
        ArrayRemove(marketListings, listing);
        
        // Notify both parties
        let companion = GetCompanion(listing.companionId);
        let purchaseData = "companion:" + companion.displayName + ",price:" + ToString(listing.price);
        NetworkingSystem.SendToPlayer(buyerId, "companion_purchased", purchaseData);
        NetworkingSystem.SendToPlayer(listing.sellerId, "companion_sold", purchaseData);
        
        return true;
    }
    
    // Utility functions
    private static func GetCompanion(companionId: String) -> AICompanion {
        for companion in registeredCompanions {
            if Equals(companion.companionId, companionId) {
                return companion;
            }
        }
        
        let emptyCompanion: AICompanion;
        return emptyCompanion;
    }
    
    private static func GetCompanionIndex(companionId: String) -> Int32 {
        for i in Range(ArraySize(registeredCompanions)) {
            if Equals(registeredCompanions[i].companionId, companionId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func CanDeployCompanion(playerId: String, companion: AICompanion) -> Bool {
        // Check if companion belongs to player or is shared
        if !Equals(companion.ownerId, playerId) && !HasSharedAccess(playerId, companion.companionId, "deploy") {
            return false;
        }
        
        // Check companion status
        if companion.status != CompanionStatus.Available {
            return false;
        }
        
        // Check fatigue
        if companion.fatigue > 0.8 {
            return false; // Too tired
        }
        
        // Check morale
        if companion.morale < 0.2 {
            return false; // Too demoralized
        }
        
        // Check cooldown
        if GetGameTime() - companion.lastDeployment < deploymentCooldown {
            return false;
        }
        
        return true;
    }
    
    private static func GeneratePersonality() -> array<PersonalityTrait> {
        let personality: array<PersonalityTrait>;
        
        // Each companion gets 2-4 personality traits
        let traitCount = RandRange(2, 5);
        
        for i in Range(traitCount) {
            let trait = Cast<PersonalityTrait>(RandRange(0, 10));
            if !ArrayContains(personality, trait) {
                ArrayPush(personality, trait);
            }
        }
        
        return personality;
    }
    
    private static func CreateCompanionAI(companion: AICompanion) -> Void {
        let ai: CompanionAI;
        ai.companionId = companion.companionId;
        ai.aiPersonality = DetermineAIPersonality(companion.personality);
        ai.decisionMaking = DetermineDecisionMaking(companion.companionType);
        ai.combatTactics = companion.combatStyle;
        ai.socialBehavior = DetermineSocialBehavior(companion.personality);
        ai.learningRate = RandRangeF(0.5, 1.0);
        ai.adaptability = RandRangeF(0.3, 0.9);
        ai.initiative = RandRangeF(0.4, 0.8);
        ai.communicationStyle = DetermineCommunicationStyle(companion.personality);
        ai.preferredRoles = DeterminePreferredRoles(companion.companionType);
        ai.dislikes = GenerateDislikes(companion.personality);
        ai.likes = GenerateLikes(companion.personality);
        
        ArrayPush(companionAI, ai);
    }
    
    public static func GetPlayerCompanions(playerId: String) -> array<AICompanion> {
        let playerCompanions: array<AICompanion>;
        
        for companion in registeredCompanions {
            if Equals(companion.ownerId, playerId) {
                ArrayPush(playerCompanions, companion);
            }
        }
        
        return playerCompanions;
    }
    
    public static func GetSharedCompanions(playerId: String) -> array<CompanionShare> {
        let sharedCompanions: array<CompanionShare>;
        
        for share in companionShares {
            if Equals(share.sharedWithId, playerId) && share.isActive {
                ArrayPush(sharedCompanions, share);
            }
        }
        
        return sharedCompanions;
    }
    
    public static func GetCompanionMarket(category: CompanionType) -> array<CompanionMarket> {
        let marketCompanions: array<CompanionMarket>;
        
        for listing in marketListings {
            if listing.category == category || category == CompanionType.Support { // Support = all
                ArrayPush(marketCompanions, listing);
            }
        }
        
        return marketCompanions;
    }
    
    public static func GetActiveDeployments(playerId: String) -> array<CompanionDeployment> {
        let playerDeployments: array<CompanionDeployment>;
        
        for deployment in activeDeployments {
            if Equals(deployment.deployerId, playerId) {
                ArrayPush(playerDeployments, deployment);
            }
        }
        
        return playerDeployments;
    }
}