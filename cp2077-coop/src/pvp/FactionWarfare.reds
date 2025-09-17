// Large-Scale Faction Warfare System for Cyberpunk 2077 Multiplayer
// Persistent faction conflicts with territorial control and massive battles

module FactionWarfare

enum WarfareScale {
    Skirmish = 0,      // 8-16 players
    Battle = 1,        // 16-32 players
    Campaign = 2,      // 32-64 players
    War = 3           // 64+ players
}

enum FactionWarType {
    TerritorialControl = 0,
    ResourceCapture = 1,
    ConvoyAttack = 2,
    BaseAssault = 3,
    Sieges = 4,
    OpenWarfare = 5,
    Assassination = 6,
    Sabotage = 7,
    Diplomacy = 8,
    EconomicWarfare = 9
}

enum WarObjective {
    CaptureTerritory = 0,
    DefendPosition = 1,
    EliminateTargets = 2,
    DestroyAssets = 3,
    RecoverIntel = 4,
    EstablishSupplyLine = 5,
    DisruptOperations = 6,
    NeutralizeLeader = 7,
    SecureResources = 8,
    InfluencePopulation = 9
}

enum FactionAlignment {
    Corporate = 0,
    Street = 1,
    Nomad = 2,
    Netrunner = 3,
    Anarchist = 4,
    Mercenary = 5,
    Government = 6,
    Criminal = 7
}

struct WarTerritory {
    let territoryId: String;
    let name: String;
    let description: String;
    let centerLocation: Vector4;
    let boundaryPoints: array<Vector4>;
    let controllingFaction: String;
    let contestedBy: array<String>;
    let strategicValue: Int32; // 1-10
    let populationCount: Int32;
    let resourceValue: Int32;
    let defenseRating: Int32;
    let influenceLevel: Float; // 0.0-1.0
    let stabilityRating: Float; // 0.0-1.0
    let lastConflict: Float;
    let activeConflicts: array<String>;
    let economicOutput: Int32;
    let militaryStrength: Int32;
}

struct WarFaction {
    let factionId: String;
    let factionName: String;
    let alignment: FactionAlignment;
    let leaderPlayerId: String;
    let officers: array<String>; // High-ranking members
    let members: array<String>;
    let allies: array<String>; // Allied faction IDs
    let enemies: array<String>; // Enemy faction IDs
    let neutrals: array<String>; // Neutral faction IDs
    let controlledTerritories: array<String>;
    let totalInfluence: Float;
    let militaryPower: Int32;
    let economicPower: Int32;
    let reputation: Float;
    let warScore: Int32;
    let activeWars: array<String>;
    let treasury: Int32;
    let supplies: Int32;
    let morale: Float;
    let recruitmentRate: Float;
    let lastActivity: Float;
}

struct WarCampaign {
    let campaignId: String;
    let campaignName: String;
    let warType: FactionWarType;
    let scale: WarfareScale;
    let attackingFactions: array<String>;
    let defendingFactions: array<String>;
    let neutralFactions: array<String>;
    let targetTerritories: array<String>;
    let objectives: array<WarObjective>;
    let startTime: Float;
    let plannedDuration: Float;
    let actualDuration: Float;
    let currentPhase: String; // "mobilization", "active", "decisive", "resolution"
    let participants: array<String>; // All player IDs
    let casualties: array<String>; // Eliminated players
    let victoryConditions: array<String>;
    let defeatConditions: array<String>;
    let currentStatus: String;
    let warScore: array<Int32>; // Score per faction
    let economicImpact: Int32;
    let politicalImpact: Float;
    let isPublic: Bool; // Open to all players
    let maxParticipants: Int32;
}

struct WarBattle {
    let battleId: String;
    let campaignId: String;
    let battleName: String;
    let location: Vector4;
    let battleType: String; // "skirmish", "siege", "assault", "defense"
    let attackers: array<String>;
    let defenders: array<String>;
    let objectives: array<String>;
    let startTime: Float;
    let duration: Float;
    let outcome: String; // "attacking_victory", "defending_victory", "stalemate"
    let casualties: array<String>;
    let territoryChanges: array<String>;
    let resourceChanges: array<Int32>;
    let experienceGained: array<Int32>;
    let tacticalSituation: String;
    let weatherConditions: String;
    let timeOfDay: String;
}

struct WarEvent {
    let eventId: String;
    let campaignId: String;
    let eventType: String; // "battle_started", "territory_captured", "leader_killed", etc.
    let timestamp: Float;
    let location: Vector4;
    let factionInvolved: String;
    let playersInvolved: array<String>;
    let eventDescription: String;
    let strategicImpact: Int32; // -5 to +5
    let economicImpact: Int32;
    let moraleImpact: Float;
    let propagandaValue: Int32;
}

struct WarSupply {
    let supplyId: String;
    let factionId: String;
    let supplyType: String; // "weapons", "ammunition", "medical", "equipment", "intel"
    let quantity: Int32;
    let quality: Int32; // 1-5
    let location: Vector4;
    let isSecured: Bool;
    let transportRoute: array<Vector4>;
    let estimatedArrival: Float;
    let guardsAssigned: array<String>;
    let threatLevel: Int32;
}

class FactionWarfare {
    private static let instance: ref<FactionWarfare>;
    private static let registeredFactions: array<WarFaction>;
    private static let warTerritories: array<WarTerritory>;
    private static let activeCampaigns: array<WarCampaign>;
    private static let activeBattles: array<WarBattle>;
    private static let warHistory: array<WarCampaign>;
    private static let recentEvents: array<WarEvent>;
    private static let activeSupplyRuns: array<WarSupply>;
    
    // System configuration
    private static let maxActiveCampaigns: Int32 = 10;
    private static let maxFactionMembers: Int32 = 100;
    private static let territoryInfluenceRadius: Float = 500.0;
    private static let campaignCooldown: Float = 3600.0; // 1 hour between campaigns
    private static let battleDuration: Float = 1800.0; // 30 minutes default
    
    public static func Initialize() -> Void {
        if IsDefined(instance) {
            return;
        }
        instance = new FactionWarfare();
        InitializeTerritories();
        InitializeNPCFactions();
        LogChannel(n"FactionWarfare", "Large-scale faction warfare system initialized");
    }
    
    private static func InitializeTerritories() -> Void {
        // Watson District
        let watson: WarTerritory;
        watson.territoryId = "watson_industrial";
        watson.name = "Watson Industrial District";
        watson.description = "Heavy industrial zone with manufacturing facilities";
        watson.centerLocation = Vector4.Create(100.0, 200.0, 25.0, 1.0);
        watson.controllingFaction = "arasaka_corp";
        watson.strategicValue = 8;
        watson.populationCount = 150000;
        watson.resourceValue = 90000;
        watson.defenseRating = 7;
        watson.influenceLevel = 0.75;
        watson.stabilityRating = 0.85;
        watson.economicOutput = 120000;
        watson.militaryStrength = 85;
        ArrayPush(warTerritories, watson);
        
        // Westbrook
        let westbrook: WarTerritory;
        westbrook.territoryId = "westbrook_corp";
        westbrook.name = "Westbrook Corporate Plaza";
        westbrook.description = "Corporate headquarters and financial district";
        westbrook.centerLocation = Vector4.Create(-150.0, 300.0, 100.0, 1.0);
        westbrook.controllingFaction = "corpo_alliance";
        westbrook.strategicValue = 10;
        westbrook.populationCount = 80000;
        westbrook.resourceValue = 200000;
        westbrook.defenseRating = 9;
        westbrook.influenceLevel = 0.90;
        westbrook.stabilityRating = 0.95;
        westbrook.economicOutput = 250000;
        westbrook.militaryStrength = 95;
        ArrayPush(warTerritories, westbrook);
        
        // Pacifica
        let pacifica: WarTerritory;
        pacifica.territoryId = "pacifica_combat";
        pacifica.name = "Pacifica Combat Zone";
        pacifica.description = "Abandoned district controlled by gangs";
        pacifica.centerLocation = Vector4.Create(200.0, -100.0, 15.0, 1.0);
        pacifica.controllingFaction = "voodoo_boys";
        pacifica.strategicValue = 6;
        pacifica.populationCount = 45000;
        pacifica.resourceValue = 30000;
        pacifica.defenseRating = 4;
        pacifica.influenceLevel = 0.60;
        pacifica.stabilityRating = 0.30;
        pacifica.economicOutput = 25000;
        pacifica.militaryStrength = 70;
        ArrayPush(warTerritories, pacifica);
        
        // Heywood
        let heywood: WarTerritory;
        heywood.territoryId = "heywood_residential";
        heywood.name = "Heywood Residential";
        heywood.description = "Mixed residential and commercial areas";
        heywood.centerLocation = Vector4.Create(-200.0, -150.0, 30.0, 1.0);
        heywood.controllingFaction = "valentinos";
        heywood.strategicValue = 7;
        heywood.populationCount = 120000;
        heywood.resourceValue = 75000;
        heywood.defenseRating = 5;
        heywood.influenceLevel = 0.70;
        heywood.stabilityRating = 0.65;
        heywood.economicOutput = 85000;
        heywood.militaryStrength = 60;
        ArrayPush(warTerritories, heywood);
        
        // Badlands
        let badlands: WarTerritory;
        badlands.territoryId = "badlands_north";
        badlands.name = "Northern Badlands";
        badlands.description = "Nomad territory with mobile settlements";
        badlands.centerLocation = Vector4.Create(500.0, 400.0, 5.0, 1.0);
        badlands.controllingFaction = "aldecaldos";
        badlands.strategicValue = 5;
        badlands.populationCount = 25000;
        badlands.resourceValue = 40000;
        badlands.defenseRating = 6;
        badlands.influenceLevel = 0.80;
        badlands.stabilityRating = 0.70;
        badlands.economicOutput = 45000;
        badlands.militaryStrength = 75;
        ArrayPush(warTerritories, badlands);
        
        // Santo Domingo
        let santo: WarTerritory;
        santo.territoryId = "santo_domingo";
        santo.name = "Santo Domingo";
        santo.description = "Industrial and residential mix with corpo presence";
        santo.centerLocation = Vector4.Create(0.0, 500.0, 35.0, 1.0);
        santo.controllingFaction = "biotechnica";
        santo.strategicValue = 8;
        santo.populationCount = 200000;
        santo.resourceValue = 110000;
        santo.defenseRating = 7;
        santo.influenceLevel = 0.65;
        santo.stabilityRating = 0.75;
        santo.economicOutput = 95000;
        santo.militaryStrength = 80;
        ArrayPush(warTerritories, santo);
    }
    
    private static func InitializeNPCFactions() -> Void {
        // Arasaka Corporation
        let arasaka: WarFaction;
        arasaka.factionId = "arasaka_corp";
        arasaka.factionName = "Arasaka Corporation";
        arasaka.alignment = FactionAlignment.Corporate;
        arasaka.totalInfluence = 85.0;
        arasaka.militaryPower = 90;
        arasaka.economicPower = 95;
        arasaka.reputation = 70.0;
        arasaka.treasury = 1000000;
        arasaka.supplies = 800;
        arasaka.morale = 0.85;
        ArrayPush(arasaka.enemies, "voodoo_boys");
        ArrayPush(arasaka.enemies, "maelstrom");
        ArrayPush(arasaka.controlledTerritories, "watson_industrial");
        ArrayPush(registeredFactions, arasaka);
        
        // Voodoo Boys
        let voodoo: WarFaction;
        voodoo.factionId = "voodoo_boys";
        voodoo.factionName = "Voodoo Boys";
        voodoo.alignment = FactionAlignment.Netrunner;
        voodoo.totalInfluence = 45.0;
        voodoo.militaryPower = 60;
        voodoo.economicPower = 40;
        voodoo.reputation = 55.0;
        voodoo.treasury = 150000;
        voodoo.supplies = 300;
        voodoo.morale = 0.75;
        ArrayPush(voodoo.enemies, "arasaka_corp");
        ArrayPush(voodoo.enemies, "corpo_alliance");
        ArrayPush(voodoo.controlledTerritories, "pacifica_combat");
        ArrayPush(registeredFactions, voodoo);
        
        // Aldecaldos
        let aldecaldos: WarFaction;
        aldecaldos.factionId = "aldecaldos";
        aldecaldos.factionName = "Aldecaldos Nomad Family";
        aldecaldos.alignment = FactionAlignment.Nomad;
        aldecaldos.totalInfluence = 60.0;
        aldecaldos.militaryPower = 75;
        aldecaldos.economicPower = 55;
        aldecaldos.reputation = 80.0;
        aldecaldos.treasury = 300000;
        aldecaldos.supplies = 500;
        aldecaldos.morale = 0.90;
        ArrayPush(aldecaldos.enemies, "biotechnica");
        ArrayPush(aldecaldos.allies, "nomad_coalition");
        ArrayPush(aldecaldos.controlledTerritories, "badlands_north");
        ArrayPush(registeredFactions, aldecaldos);
    }
    
    // Faction management
    public static func CreatePlayerFaction(leaderId: String, factionName: String, alignment: FactionAlignment) -> String {
        if IsPlayerInFaction(leaderId) {
            return "";
        }
        
        let factionId = "player_" + leaderId + "_" + ToString(GetGameTime());
        
        let faction: WarFaction;
        faction.factionId = factionId;
        faction.factionName = factionName;
        faction.alignment = alignment;
        faction.leaderPlayerId = leaderId;
        ArrayPush(faction.members, leaderId);
        faction.totalInfluence = 10.0;
        faction.militaryPower = 25;
        faction.economicPower = 20;
        faction.reputation = 50.0;
        faction.warScore = 0;
        faction.treasury = 50000;
        faction.supplies = 100;
        faction.morale = 0.80;
        faction.recruitmentRate = 1.0;
        faction.lastActivity = GetGameTime();
        
        ArrayPush(registeredFactions, faction);
        
        let factionData = JsonStringify(faction);
        NetworkingSystem.SendToPlayer(leaderId, "faction_created", factionData);
        NetworkingSystem.BroadcastMessage("faction_announced", factionData);
        
        LogChannel(n"FactionWarfare", StrCat("Created player faction: ", factionId));
        return factionId;
    }
    
    public static func JoinFaction(playerId: String, factionId: String) -> Bool {
        if IsPlayerInFaction(playerId) {
            return false;
        }
        
        let factionIndex = GetFactionIndex(factionId);
        if factionIndex == -1 {
            return false;
        }
        
        let faction = registeredFactions[factionIndex];
        if ArraySize(faction.members) >= maxFactionMembers {
            return false;
        }
        
        // Reputation check
        if !MeetsReputationRequirement(playerId, faction) {
            return false;
        }
        
        ArrayPush(faction.members, playerId);
        faction.militaryPower += GetPlayerMilitaryContribution(playerId);
        faction.economicPower += GetPlayerEconomicContribution(playerId);
        
        registeredFactions[factionIndex] = faction;
        
        let joinData = "faction:" + factionId + ",player:" + playerId;
        NetworkingSystem.SendToPlayer(playerId, "faction_joined", joinData);
        BroadcastToFaction(factionId, "member_joined", joinData);
        
        return true;
    }
    
    // War campaign management
    public static func DeclareCampaign(initiatorFactionId: String, targetFactionId: String, warType: FactionWarType, targetTerritories: array<String>) -> String {
        if !CanDeclareCampaign(initiatorFactionId, targetFactionId) {
            return "";
        }
        
        if ArraySize(activeCampaigns) >= maxActiveCampaigns {
            return "";
        }
        
        let campaignId = "campaign_" + initiatorFactionId + "_" + ToString(GetGameTime());
        
        let campaign: WarCampaign;
        campaign.campaignId = campaignId;
        campaign.campaignName = GenerateCampaignName(warType, targetTerritories);
        campaign.warType = warType;
        campaign.scale = CalculateCampaignScale(initiatorFactionId, targetFactionId);
        ArrayPush(campaign.attackingFactions, initiatorFactionId);
        ArrayPush(campaign.defendingFactions, targetFactionId);
        campaign.targetTerritories = targetTerritories;
        campaign.objectives = GenerateWarObjectives(warType, targetTerritories);
        campaign.startTime = GetGameTime() + 600.0; // 10 minute mobilization
        campaign.plannedDuration = CalculateCampaignDuration(campaign.scale, warType);
        campaign.currentPhase = "mobilization";
        campaign.isPublic = true;
        campaign.maxParticipants = GetMaxParticipants(campaign.scale);
        campaign.currentStatus = "declared";
        
        // Initialize faction scores
        ArrayPush(campaign.warScore, 0); // Attackers
        ArrayPush(campaign.warScore, 0); // Defenders
        
        ArrayPush(activeCampaigns, campaign);
        
        // Notify all factions
        let declarationData = JsonStringify(campaign);
        NetworkingSystem.BroadcastMessage("war_campaign_declared", declarationData);
        
        // Update faction relations
        UpdateFactionRelations(initiatorFactionId, targetFactionId, "hostile");
        
        LogChannel(n"FactionWarfare", StrCat("Declared war campaign: ", campaignId));
        return campaignId;
    }
    
    public static func JoinCampaign(playerId: String, campaignId: String, side: String) -> Bool {
        let campaignIndex = GetCampaignIndex(campaignId);
        if campaignIndex == -1 {
            return false;
        }
        
        let campaign = activeCampaigns[campaignIndex];
        
        if campaign.currentPhase != "mobilization" {
            return false;
        }
        
        if ArraySize(campaign.participants) >= campaign.maxParticipants {
            return false;
        }
        
        if ArrayContains(campaign.participants, playerId) {
            return false;
        }
        
        let playerFaction = GetPlayerFaction(playerId);
        if !CanJoinSide(playerFaction, campaign, side) {
            return false;
        }
        
        ArrayPush(campaign.participants, playerId);
        
        // Add faction to appropriate side if not already there
        if Equals(side, "attacking") && !ArrayContains(campaign.attackingFactions, playerFaction) {
            ArrayPush(campaign.attackingFactions, playerFaction);
        } else if Equals(side, "defending") && !ArrayContains(campaign.defendingFactions, playerFaction) {
            ArrayPush(campaign.defendingFactions, playerFaction);
        }
        
        activeCampaigns[campaignIndex] = campaign;
        
        let joinData = "campaign:" + campaignId + ",side:" + side;
        NetworkingSystem.SendToPlayer(playerId, "campaign_joined", joinData);
        BroadcastToCampaign(campaignId, "player_joined_campaign", joinData);
        
        return true;
    }
    
    public static func StartCampaign(campaignId: String) -> Void {
        let campaignIndex = GetCampaignIndex(campaignId);
        if campaignIndex == -1 {
            return;
        }
        
        let campaign = activeCampaigns[campaignIndex];
        if campaign.currentPhase != "mobilization" {
            return;
        }
        
        campaign.currentPhase = "active";
        campaign.startTime = GetGameTime();
        campaign.currentStatus = "active";
        
        // Initialize territory contested status
        for territoryId in campaign.targetTerritories {
            MarkTerritoryContested(territoryId, campaign.attackingFactions);
        }
        
        // Start initial battles
        StartInitialBattles(campaign);
        
        activeCampaigns[campaignIndex] = campaign;
        
        BroadcastToCampaign(campaignId, "campaign_started", "");
        NetworkingSystem.BroadcastMessage("war_campaign_active", JsonStringify(campaign));
        
        LogChannel(n"FactionWarfare", StrCat("Started war campaign: ", campaignId));
    }
    
    // Battle management
    public static func InitiateBattle(campaignId: String, battleType: String, location: Vector4, attackers: array<String>, defenders: array<String>) -> String {
        let battleId = campaignId + "_battle_" + ToString(GetGameTime());
        
        let battle: WarBattle;
        battle.battleId = battleId;
        battle.campaignId = campaignId;
        battle.battleName = GenerateBattleName(battleType, location);
        battle.location = location;
        battle.battleType = battleType;
        battle.attackers = attackers;
        battle.defenders = defenders;
        battle.objectives = GenerateBattleObjectives(battleType);
        battle.startTime = GetGameTime();
        battle.duration = battleDuration;
        battle.outcome = "ongoing";
        battle.tacticalSituation = AnalyzeTacticalSituation(attackers, defenders);
        battle.weatherConditions = GetCurrentWeather();
        battle.timeOfDay = GetCurrentTimeOfDay();
        
        ArrayPush(activeBattles, battle);
        
        // Teleport participants to battle zone
        let allParticipants: array<String>;
        ArrayConcatenate(allParticipants, attackers);
        ArrayConcatenate(allParticipants, defenders);
        
        for participantId in allParticipants {
            TeleportToBattleZone(participantId, location, battleType);
        }
        
        let battleData = JsonStringify(battle);
        BroadcastToCampaign(campaignId, "battle_initiated", battleData);
        
        // Start battle timer
        StartBattleTimer(battleId);
        
        LogChannel(n"FactionWarfare", StrCat("Initiated battle: ", battleId));
        return battleId;
    }
    
    public static func OnBattleObjectiveCompleted(battleId: String, completingTeam: String, objectiveId: String) -> Void {
        let battleIndex = GetBattleIndex(battleId);
        if battleIndex == -1 {
            return;
        }
        
        let battle = activeBattles[battleIndex];
        ArrayPush(battle.objectives, objectiveId);
        
        // Award points based on objective
        let points = GetObjectivePoints(objectiveId);
        UpdateCampaignScore(battle.campaignId, completingTeam, points);
        
        activeBattles[battleIndex] = battle;
        
        let objData = "objective:" + objectiveId + ",team:" + completingTeam + ",points:" + ToString(points);
        BroadcastToBattle(battleId, "battle_objective_completed", objData);
        
        // Check if battle should end
        if ShouldEndBattle(battle) {
            EndBattle(battleId, DetermineBattleWinner(battle));
        }
    }
    
    public static func EndBattle(battleId: String, outcome: String) -> Void {
        let battleIndex = GetBattleIndex(battleId);
        if battleIndex == -1 {
            return;
        }
        
        let battle = activeBattles[battleIndex];
        battle.outcome = outcome;
        battle.duration = GetGameTime() - battle.startTime;
        
        // Calculate casualties and experience
        CalculateBattleCasualties(battle);
        DistributeBattleExperience(battle);
        
        // Apply territorial changes
        if RequiresTerritorialChange(battle, outcome) {
            ApplyTerritorialChanges(battle, outcome);
        }
        
        // Update campaign progress
        UpdateCampaignProgress(battle.campaignId, battle);
        
        // Remove from active battles
        ArrayRemove(activeBattles, battle);
        
        let endData = "outcome:" + outcome + ",duration:" + ToString(battle.duration);
        BroadcastToBattle(battleId, "battle_ended", endData);
        
        LogChannel(n"FactionWarfare", StrCat("Ended battle: ", battleId, " Outcome: ", outcome));
    }
    
    // Territory and influence management
    public static func CaptureTerritory(territoryId: String, capturingFactionId: String) -> Bool {
        let territoryIndex = GetTerritoryIndex(territoryId);
        if territoryIndex == -1 {
            return false;
        }
        
        let territory = warTerritories[territoryIndex];
        let previousController = territory.controllingFaction;
        
        // Transfer control
        territory.controllingFaction = capturingFactionId;
        territory.influenceLevel = 0.5; // Start with partial influence
        territory.stabilityRating = 0.3; // Unstable after capture
        territory.lastConflict = GetGameTime();
        
        // Update faction territories
        UpdateFactionTerritories(previousController, capturingFactionId, territoryId);
        
        warTerritories[territoryIndex] = territory;
        
        // Generate major event
        RecordWarEvent("territory_captured", "", GetGameTime(), territory.centerLocation, capturingFactionId, [], 
                      StrCat("Territory ", territory.name, " captured by ", GetFactionName(capturingFactionId)), 5, territory.economicOutput, 0.2);
        
        let captureData = "territory:" + territoryId + ",faction:" + capturingFactionId + ",previous:" + previousController;
        NetworkingSystem.BroadcastMessage("territory_captured", captureData);
        
        LogChannel(n"FactionWarfare", StrCat("Territory captured: ", territoryId, " by ", capturingFactionId));
        return true;
    }
    
    public static func UpdateTerritorialInfluence(territoryId: String, factionId: String, influenceChange: Float) -> Void {
        let territoryIndex = GetTerritoryIndex(territoryId);
        if territoryIndex == -1 {
            return;
        }
        
        let territory = warTerritories[territoryIndex];
        
        if Equals(territory.controllingFaction, factionId) {
            territory.influenceLevel += influenceChange;
            territory.influenceLevel = ClampF(territory.influenceLevel, 0.0, 1.0);
            
            // Increase stability with higher influence
            if territory.influenceLevel > 0.8 {
                territory.stabilityRating += 0.01;
                territory.stabilityRating = MinF(territory.stabilityRating, 1.0);
            }
        }
        
        warTerritories[territoryIndex] = territory;
    }
    
    // Supply and logistics
    public static func StartSupplyRun(factionId: String, supplyType: String, quantity: Int32, destination: Vector4, guards: array<String>) -> String {
        let supplyId = factionId + "_supply_" + ToString(GetGameTime());
        
        let supply: WarSupply;
        supply.supplyId = supplyId;
        supply.factionId = factionId;
        supply.supplyType = supplyType;
        supply.quantity = quantity;
        supply.quality = CalculateSupplyQuality(factionId, supplyType);
        supply.location = GetFactionSupplyBase(factionId);
        supply.isSecured = true;
        supply.transportRoute = CalculateSupplyRoute(supply.location, destination);
        supply.estimatedArrival = GetGameTime() + CalculateSupplyTravelTime(supply.transportRoute);
        supply.guardsAssigned = guards;
        supply.threatLevel = CalculateSupplyThreatLevel(supply.transportRoute);
        
        ArrayPush(activeSupplyRuns, supply);
        
        // Notify potential interceptors
        NotifySupplyThreat(supply);
        
        let supplyData = JsonStringify(supply);
        BroadcastToFaction(factionId, "supply_run_started", supplyData);
        
        LogChannel(n"FactionWarfare", StrCat("Started supply run: ", supplyId));
        return supplyId;
    }
    
    public static func InterceptSupplyRun(supplyId: String, interceptingPlayerId: String) -> Bool {
        let supplyIndex = GetSupplyRunIndex(supplyId);
        if supplyIndex == -1 {
            return false;
        }
        
        let supply = activeSupplyRuns[supplyIndex];
        let interceptorFaction = GetPlayerFaction(interceptingPlayerId);
        
        if Equals(interceptorFaction, supply.factionId) {
            return false; // Can't intercept own supplies
        }
        
        if supply.isSecured {
            return false; // Already secured
        }
        
        // Start interception battle
        let interceptLocation = GetCurrentSupplyLocation(supply);
        let defenders = supply.guardsAssigned;
        let attackers: array<String>;
        ArrayPush(attackers, interceptingPlayerId);
        
        let battleId = InitiateBattle("supply_interception", "convoy_attack", interceptLocation, attackers, defenders);
        
        // Link battle to supply run
        LinkBattleToSupply(battleId, supplyId);
        
        return true;
    }
    
    // Utility functions
    private static func GetFactionIndex(factionId: String) -> Int32 {
        for i in Range(ArraySize(registeredFactions)) {
            if Equals(registeredFactions[i].factionId, factionId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func GetCampaignIndex(campaignId: String) -> Int32 {
        for i in Range(ArraySize(activeCampaigns)) {
            if Equals(activeCampaigns[i].campaignId, campaignId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func GetBattleIndex(battleId: String) -> Int32 {
        for i in Range(ArraySize(activeBattles)) {
            if Equals(activeBattles[i].battleId, battleId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func GetTerritoryIndex(territoryId: String) -> Int32 {
        for i in Range(ArraySize(warTerritories)) {
            if Equals(warTerritories[i].territoryId, territoryId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func IsPlayerInFaction(playerId: String) -> Bool {
        for faction in registeredFactions {
            if ArrayContains(faction.members, playerId) {
                return true;
            }
        }
        return false;
    }
    
    private static func GetPlayerFaction(playerId: String) -> String {
        for faction in registeredFactions {
            if ArrayContains(faction.members, playerId) {
                return faction.factionId;
            }
        }
        return "";
    }
    
    private static func CanDeclareCampaign(initiatorId: String, targetId: String) -> Bool {
        // Can't declare war on yourself
        if Equals(initiatorId, targetId) {
            return false;
        }
        
        // Check if factions exist
        if GetFactionIndex(initiatorId) == -1 || GetFactionIndex(targetId) == -1 {
            return false;
        }
        
        // Check if already at war
        for campaign in activeCampaigns {
            if (ArrayContains(campaign.attackingFactions, initiatorId) && ArrayContains(campaign.defendingFactions, targetId)) ||
               (ArrayContains(campaign.attackingFactions, targetId) && ArrayContains(campaign.defendingFactions, initiatorId)) {
                return false;
            }
        }
        
        // Check cooldown
        let lastCampaign = GetLastCampaignTime(initiatorId);
        if GetGameTime() - lastCampaign < campaignCooldown {
            return false;
        }
        
        return true;
    }
    
    public static func GetActiveCampaigns() -> array<WarCampaign> {
        return activeCampaigns;
    }
    
    public static func GetActiveBattles() -> array<WarBattle> {
        return activeBattles;
    }
    
    public static func GetTerritorialControl() -> array<WarTerritory> {
        return warTerritories;
    }
    
    public static func GetFactionStats(factionId: String) -> WarFaction {
        for faction in registeredFactions {
            if Equals(faction.factionId, factionId) {
                return faction;
            }
        }
        
        let emptyFaction: WarFaction;
        return emptyFaction;
    }
}