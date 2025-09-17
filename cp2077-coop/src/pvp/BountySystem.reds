// Bounty Hunting System for Cyberpunk 2077 Multiplayer
// Player vs Player assassination contracts with reputation system

module BountySystem

enum BountyType {
    StandardKill = 0,
    StealthKill = 1,
    NetrunnerHit = 2,
    VehicleElimination = 3,
    ExplosiveElimination = 4,
    MeleeOnly = 5,
    NoImplants = 6,
    PublicExecution = 7,
    TimeLimit = 8,
    CaptureAlive = 9
}

enum BountyStatus {
    Posted = 0,
    Accepted = 1,
    InProgress = 2,
    Completed = 3,
    Failed = 4,
    Expired = 5,
    Cancelled = 6
}

enum BountyRank {
    Novice = 0,
    Experienced = 1,
    Professional = 2,
    Elite = 3,
    Legendary = 4
}

enum ContractDifficulty {
    Easy = 0,
    Medium = 1,
    Hard = 2,
    Extreme = 3,
    Nightmare = 4
}

struct BountyTarget {
    let targetId: String;
    let targetName: String;
    let lastKnownLocation: Vector4;
    let lastSeenTime: Float;
    let threatLevel: Int32; // 1-10
    let preferredWeapons: array<String>;
    let knownCyberware: array<String>;
    let bountyHistory: Int32; // Times they've been targeted
    let escapeAttempts: Int32;
    let killStreak: Int32;
    let notoriety: Float;
    let activeWarnings: Int32; // How many times they've been warned
}

struct BountyContract {
    let contractId: String;
    let posterId: String;
    let targetId: String;
    let hunterId: String;
    let bountyType: BountyType;
    let difficulty: ContractDifficulty;
    let reward: Int32;
    let timeLimit: Float;
    let postTime: Float;
    let acceptTime: Float;
    let completionTime: Float;
    let status: BountyStatus;
    let requirements: array<String>;
    let restrictions: array<String>;
    let evidence: array<String>; // Screenshots, videos, etc.
    let witnesses: array<String>;
    let bonusConditions: array<String>;
    let bonusReward: Int32;
    let failurePenalty: Int32;
    let isPublic: Bool; // Visible to all players
    let allowedHunters: array<String>; // Specific hunters only
    let maxHunters: Int32; // Multiple hunters can accept
    let currentHunters: array<String>;
}

struct BountyHunter {
    let hunterId: String;
    let hunterName: String;
    let rank: BountyRank;
    let reputation: Float;
    let totalContracts: Int32;
    let successfulContracts: Int32;
    let failedContracts: Int32;
    let successRate: Float;
    let totalEarnings: Int32;
    let averageCompletionTime: Float;
    let specializations: array<BountyType>;
    let preferredDifficulty: ContractDifficulty;
    let activeContracts: array<String>;
    let isAcceptingContracts: Bool;
    let lastActivityTime: Float;
    let notorietyScore: Float;
    let hunterRating: Float; // Player reviews
}

struct BountyEvidence {
    let evidenceId: String;
    let contractId: String;
    let evidenceType: String; // "screenshot", "video", "witness", "location"
    let timestamp: Float;
    let description: String;
    let submitterId: String;
    let isVerified: Bool;
    let verificationScore: Float;
}

struct BountyEvent {
    let eventId: String;
    let contractId: String;
    let eventType: String; // "target_spotted", "hunter_killed", "escape_attempt", "contract_completed"
    let timestamp: Float;
    let location: Vector4;
    let involvedPlayers: array<String>;
    let details: String;
    let notificationSent: Bool;
}

class BountySystem {
    private static let instance: ref<BountySystem>;
    private static let activeContracts: array<BountyContract>;
    private static let registeredHunters: array<BountyHunter>;
    private static let bountyTargets: array<BountyTarget>;
    private static let contractHistory: array<BountyContract>;
    private static let evidence: array<BountyEvidence>;
    private static let recentEvents: array<BountyEvent>;
    
    // Bounty board settings
    private static let maxActiveContracts: Int32 = 50;
    private static let contractExpirationTime: Float = 3600.0; // 1 hour
    private static let hunterCooldownTime: Float = 300.0; // 5 minutes between contracts
    private static let targetImmunityTime: Float = 600.0; // 10 minutes after completing contract
    
    public static func Initialize() -> Void {
        if IsDefined(instance) {
            return;
        }
        instance = new BountySystem();
        LogChannel(n"BountySystem", "Bounty hunting system initialized");
    }
    
    // Contract posting and management
    public static func PostBountyContract(posterId: String, targetId: String, bountyType: BountyType, reward: Int32, timeLimit: Float, requirements: array<String>) -> String {
        if !CanPostBounty(posterId, targetId) {
            return "";
        }
        
        if ArraySize(activeContracts) >= maxActiveContracts {
            return "";
        }
        
        let contractId = "bounty_" + posterId + "_" + targetId + "_" + ToString(GetGameTime());
        
        let contract: BountyContract;
        contract.contractId = contractId;
        contract.posterId = posterId;
        contract.targetId = targetId;
        contract.hunterId = "";
        contract.bountyType = bountyType;
        contract.difficulty = CalculateContractDifficulty(targetId, bountyType);
        contract.reward = ValidateReward(reward, contract.difficulty);
        contract.timeLimit = timeLimit;
        contract.postTime = GetGameTime();
        contract.acceptTime = 0.0;
        contract.completionTime = 0.0;
        contract.status = BountyStatus.Posted;
        contract.requirements = requirements;
        contract.bonusReward = CalculateBonusReward(contract.difficulty, bountyType);
        contract.failurePenalty = CalculateFailurePenalty(contract.reward);
        contract.isPublic = true;
        contract.maxHunters = GetMaxHuntersForType(bountyType);
        
        ArrayPush(activeContracts, contract);
        
        // Update target information
        UpdateTargetInfo(targetId);
        
        // Notify potential hunters
        NotifyEligibleHunters(contract);
        
        let contractData = JsonStringify(contract);
        NetworkingSystem.BroadcastMessage("bounty_contract_posted", contractData);
        
        LogChannel(n"BountySystem", StrCat("Posted bounty contract: ", contractId));
        return contractId;
    }
    
    public static func AcceptBountyContract(hunterId: String, contractId: String) -> Bool {
        let contractIndex = GetContractIndex(contractId);
        if contractIndex == -1 {
            return false;
        }
        
        let contract = activeContracts[contractIndex];
        if contract.status != BountyStatus.Posted {
            return false;
        }
        
        if !CanAcceptContract(hunterId, contract) {
            return false;
        }
        
        if ArraySize(contract.currentHunters) >= contract.maxHunters {
            return false;
        }
        
        // Update contract
        if ArraySize(contract.currentHunters) == 0 {
            contract.hunterId = hunterId;
            contract.acceptTime = GetGameTime();
            contract.status = BountyStatus.Accepted;
        }
        
        ArrayPush(contract.currentHunters, hunterId);
        activeContracts[contractIndex] = contract;
        
        // Update hunter info
        let hunterIndex = GetHunterIndex(hunterId);
        if hunterIndex != -1 {
            ArrayPush(registeredHunters[hunterIndex].activeContracts, contractId);
        }
        
        // Notify target
        let targetData = "hunter:" + hunterId + ",contract:" + contractId;
        NetworkingSystem.SendToPlayer(contract.targetId, "bounty_hunter_assigned", targetData);
        
        // Start tracking
        StartContractTracking(contractId);
        
        let acceptData = JsonStringify(contract);
        NetworkingSystem.SendToPlayer(hunterId, "bounty_contract_accepted", acceptData);
        NetworkingSystem.BroadcastMessage("bounty_contract_updated", acceptData);
        
        return true;
    }
    
    public static func CompleteContract(contractId: String, hunterId: String, evidenceIds: array<String>) -> Bool {
        let contractIndex = GetContractIndex(contractId);
        if contractIndex == -1 {
            return false;
        }
        
        let contract = activeContracts[contractIndex];
        if !ArrayContains(contract.currentHunters, hunterId) {
            return false;
        }
        
        if !ValidateContractCompletion(contract, hunterId, evidenceIds) {
            return false;
        }
        
        contract.status = BountyStatus.Completed;
        contract.completionTime = GetGameTime();
        contract.evidence = evidenceIds;
        
        // Calculate final reward
        let finalReward = CalculateFinalReward(contract, hunterId);
        let bonusEarned = CheckBonusConditions(contract, hunterId);
        
        // Pay hunter
        EconomySystem.TransferFunds(contract.posterId, hunterId, finalReward + bonusEarned);
        
        // Update statistics
        UpdateHunterStats(hunterId, contract, true);
        UpdateTargetStats(contract.targetId, false);
        
        // Move to history
        ArrayPush(contractHistory, contract);
        ArrayRemove(activeContracts, contract);
        
        // Apply target immunity period
        ApplyTargetImmunity(contract.targetId);
        
        let completionData = "reward:" + ToString(finalReward + bonusEarned) + ",bonus:" + ToString(bonusEarned);
        NetworkingSystem.SendToPlayer(hunterId, "bounty_contract_completed", completionData);
        NetworkingSystem.SendToPlayer(contract.targetId, "bounty_contract_completed_target", "");
        NetworkingSystem.BroadcastMessage("bounty_contract_completed_public", JsonStringify(contract));
        
        LogChannel(n"BountySystem", StrCat("Completed bounty contract: ", contractId, " Hunter: ", hunterId));
        return true;
    }
    
    public static func FailContract(contractId: String, hunterId: String, reason: String) -> Void {
        let contractIndex = GetContractIndex(contractId);
        if contractIndex == -1 {
            return;
        }
        
        let contract = activeContracts[contractIndex];
        contract.status = BountyStatus.Failed;
        
        // Apply failure penalty
        EconomySystem.ChargePenalty(hunterId, contract.failurePenalty);
        
        // Update statistics
        UpdateHunterStats(hunterId, contract, false);
        
        // Remove hunter from contract
        ArrayRemove(contract.currentHunters, hunterId);
        
        // If no hunters left, expire contract
        if ArraySize(contract.currentHunters) == 0 {
            contract.status = BountyStatus.Expired;
            ArrayPush(contractHistory, contract);
            ArrayRemove(activeContracts, contract);
        } else {
            contract.status = BountyStatus.InProgress;
            activeContracts[contractIndex] = contract;
        }
        
        let failData = "reason:" + reason + ",penalty:" + ToString(contract.failurePenalty);
        NetworkingSystem.SendToPlayer(hunterId, "bounty_contract_failed", failData);
        
        LogChannel(n"BountySystem", StrCat("Failed bounty contract: ", contractId, " Hunter: ", hunterId, " Reason: ", reason));
    }
    
    // Hunter registration and management
    public static func RegisterAsHunter(playerId: String) -> Bool {
        if IsRegisteredHunter(playerId) {
            return false;
        }
        
        let hunter: BountyHunter;
        hunter.hunterId = playerId;
        hunter.hunterName = PlayerSystem.GetPlayerName(playerId);
        hunter.rank = BountyRank.Novice;
        hunter.reputation = 100.0;
        hunter.totalContracts = 0;
        hunter.successfulContracts = 0;
        hunter.failedContracts = 0;
        hunter.successRate = 0.0;
        hunter.totalEarnings = 0;
        hunter.averageCompletionTime = 0.0;
        hunter.preferredDifficulty = ContractDifficulty.Easy;
        hunter.isAcceptingContracts = true;
        hunter.lastActivityTime = GetGameTime();
        hunter.notorietyScore = 0.0;
        hunter.hunterRating = 5.0;
        
        ArrayPush(registeredHunters, hunter);
        
        NetworkingSystem.SendToPlayer(playerId, "bounty_hunter_registered", "");
        LogChannel(n"BountySystem", StrCat("Registered new bounty hunter: ", playerId));
        return true;
    }
    
    public static func UpdateHunterSpecialization(hunterId: String, specialization: BountyType) -> Void {
        let hunterIndex = GetHunterIndex(hunterId);
        if hunterIndex == -1 {
            return;
        }
        
        if !ArrayContains(registeredHunters[hunterIndex].specializations, specialization) {
            ArrayPush(registeredHunters[hunterIndex].specializations, specialization);
        }
        
        let specData = ToString(Cast<Int32>(specialization));
        NetworkingSystem.SendToPlayer(hunterId, "hunter_specialization_added", specData);
    }
    
    // Target tracking and events
    public static func OnPlayerKilled(victimId: String, killerId: String, location: Vector4) -> Void {
        // Check if this was a bounty kill
        for contract in activeContracts {
            if Equals(contract.targetId, victimId) && ArrayContains(contract.currentHunters, killerId) {
                if ValidateKillConditions(contract, killerId, location) {
                    RecordBountyEvent(contract.contractId, "target_eliminated", location, [killerId, victimId]);
                    
                    // Auto-complete if simple kill contract
                    if contract.bountyType == BountyType.StandardKill {
                        let evidence: array<String>;
                        ArrayPush(evidence, GenerateKillEvidence(contract.contractId, killerId, victimId, location));
                        CompleteContract(contract.contractId, killerId, evidence);
                    }
                }
            }
        }
        
        // Check if a hunter was killed by their target
        for contract in activeContracts {
            if Equals(contract.targetId, killerId) && ArrayContains(contract.currentHunters, victimId) {
                RecordBountyEvent(contract.contractId, "hunter_eliminated", location, [killerId, victimId]);
                FailContract(contract.contractId, victimId, "killed_by_target");
            }
        }
    }
    
    public static func OnPlayerLocationUpdate(playerId: String, location: Vector4) -> Void {
        // Update target last known location
        UpdateTargetLocation(playerId, location);
        
        // Check for proximity alerts
        for contract in activeContracts {
            if Equals(contract.targetId, playerId) {
                for hunterId in contract.currentHunters {
                    let hunterLocation = PlayerSystem.GetPlayerLocation(hunterId);
                    let distance = Vector4.Distance(location, hunterLocation);
                    
                    if distance < 100.0 { // 100 meter proximity
                        SendProximityAlert(contract.contractId, hunterId, playerId, distance);
                    }
                }
            }
        }
    }
    
    // Evidence and verification system
    public static func SubmitEvidence(contractId: String, evidenceType: String, description: String, submitterId: String) -> String {
        let evidenceId = contractId + "_evidence_" + ToString(GetGameTime());
        
        let evidence: BountyEvidence;
        evidence.evidenceId = evidenceId;
        evidence.contractId = contractId;
        evidence.evidenceType = evidenceType;
        evidence.timestamp = GetGameTime();
        evidence.description = description;
        evidence.submitterId = submitterId;
        evidence.isVerified = false;
        evidence.verificationScore = 0.0;
        
        ArrayPush(this.evidence, evidence);
        
        // Auto-verify certain types
        if Equals(evidenceType, "system_generated") {
            evidence.isVerified = true;
            evidence.verificationScore = 1.0;
        }
        
        let evidenceData = JsonStringify(evidence);
        NetworkingSystem.SendToPlayer(submitterId, "evidence_submitted", evidenceData);
        
        return evidenceId;
    }
    
    public static func VerifyEvidence(evidenceId: String, verifierId: String, isValid: Bool) -> Void {
        for i in Range(ArraySize(evidence)) {
            if Equals(evidence[i].evidenceId, evidenceId) {
                if isValid {
                    evidence[i].verificationScore += 0.1;
                } else {
                    evidence[i].verificationScore -= 0.1;
                }
                
                evidence[i].verificationScore = ClampF(evidence[i].verificationScore, 0.0, 1.0);
                
                if evidence[i].verificationScore >= 0.7 {
                    evidence[i].isVerified = true;
                }
                
                break;
            }
        }
    }
    
    // Utility functions
    private static func CanPostBounty(posterId: String, targetId: String) -> Bool {
        // Can't target yourself
        if Equals(posterId, targetId) {
            return false;
        }
        
        // Check if target has immunity
        if HasTargetImmunity(targetId) {
            return false;
        }
        
        // Check if player has funds for minimum reward
        let minReward = GetMinimumReward();
        if !EconomySystem.HasFunds(posterId, minReward) {
            return false;
        }
        
        // Check existing contracts on this target
        let existingContracts = 0;
        for contract in activeContracts {
            if Equals(contract.targetId, targetId) {
                existingContracts += 1;
            }
        }
        
        return existingContracts < 3; // Max 3 contracts per target
    }
    
    private static func CanAcceptContract(hunterId: String, contract: BountyContract) -> Bool {
        let hunter = GetHunterById(hunterId);
        if !IsDefined(hunter) {
            return false;
        }
        
        // Check if hunter is on cooldown
        if IsHunterOnCooldown(hunterId) {
            return false;
        }
        
        // Check rank requirements
        let requiredRank = GetRequiredRankForDifficulty(contract.difficulty);
        if Cast<Int32>(hunter.rank) < Cast<Int32>(requiredRank) {
            return false;
        }
        
        // Check active contract limit
        if ArraySize(hunter.activeContracts) >= GetMaxActiveContractsForRank(hunter.rank) {
            return false;
        }
        
        // Can't hunt yourself
        if Equals(hunterId, contract.targetId) {
            return false;
        }
        
        return true;
    }
    
    private static func CalculateContractDifficulty(targetId: String, bountyType: BountyType) -> ContractDifficulty {
        let target = GetTargetById(targetId);
        let baseDifficulty = target.threatLevel;
        
        // Adjust for bounty type
        switch bountyType {
            case BountyType.StealthKill: baseDifficulty += 2;
            case BountyType.MeleeOnly: baseDifficulty += 3;
            case BountyType.NoImplants: baseDifficulty += 4;
            case BountyType.CaptureAlive: baseDifficulty += 5;
            default: break;
        }
        
        // Adjust for target history
        baseDifficulty += target.escapeAttempts;
        baseDifficulty += (target.bountyHistory / 2);
        
        if baseDifficulty <= 3 {
            return ContractDifficulty.Easy;
        } else if baseDifficulty <= 6 {
            return ContractDifficulty.Medium;
        } else if baseDifficulty <= 9 {
            return ContractDifficulty.Hard;
        } else if baseDifficulty <= 12 {
            return ContractDifficulty.Extreme;
        } else {
            return ContractDifficulty.Nightmare;
        }
    }
    
    private static func GetContractIndex(contractId: String) -> Int32 {
        for i in Range(ArraySize(activeContracts)) {
            if Equals(activeContracts[i].contractId, contractId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func GetHunterIndex(hunterId: String) -> Int32 {
        for i in Range(ArraySize(registeredHunters)) {
            if Equals(registeredHunters[i].hunterId, hunterId) {
                return i;
            }
        }
        return -1;
    }
    
    public static func GetActiveContracts() -> array<BountyContract> {
        return activeContracts;
    }
    
    public static func GetHunterStats(hunterId: String) -> BountyHunter {
        for hunter in registeredHunters {
            if Equals(hunter.hunterId, hunterId) {
                return hunter;
            }
        }
        
        let emptyHunter: BountyHunter;
        return emptyHunter;
    }
    
    public static func GetContractHistory(playerId: String) -> array<BountyContract> {
        let history: array<BountyContract>;
        
        for contract in contractHistory {
            if Equals(contract.posterId, playerId) || Equals(contract.targetId, playerId) || ArrayContains(contract.currentHunters, playerId) {
                ArrayPush(history, contract);
            }
        }
        
        return history;
    }
}