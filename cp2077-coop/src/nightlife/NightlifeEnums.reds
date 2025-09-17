// Nightlife System Enumerations and Supporting Types
// Supporting data structures for interactive nightlife venues and social activities

public enum BarType {
    Cocktail,
    Sports,
    Wine,
    Whiskey,
    Beer,
    Tiki,
    Rooftop,
    Underground,
    Hotel,
    Neighborhood,
    Craft,
    Dive,
    Upscale,
    Theme,
    Gaming
}

public enum DanceStyle {
    Freestyle,
    Hip Hop,
    Breakdance,
    Salsa,
    Tango,
    Electronic,
    Swing,
    Contemporary,
    Ballroom,
    Street,
    Latin,
    Techno,
    House,
    Dubstep,
    Synchronized
}

public enum InteractionType {
    Conversation,
    Dancing,
    Drinking,
    Gaming,
    Flirting,
    Networking,
    Competition,
    Collaboration,
    Performance,
    Teaching,
    Learning,
    Celebrating,
    Complaining,
    Gossiping,
    Planning
}

public enum PrivacyLevel {
    Public,
    Semi Private,
    Private,
    VIP,
    Executive,
    Celebrity,
    Invitation Only,
    Members Only,
    Ultra Private,
    Exclusive
}

public enum ServiceLevel {
    Basic,
    Standard,
    Premium,
    Luxury,
    Concierge,
    Butler,
    Celebrity,
    Royal,
    Custom,
    Unlimited
}

public enum ViewType {
    City Skyline,
    Ocean View,
    Mountain View,
    Street Level,
    Garden View,
    Dance Floor,
    Stage View,
    Bar View,
    Private Courtyard,
    Rooftop Panorama
}

public enum KaraokeSessionType {
    Solo,
    Duet,
    Group,
    Competition,
    Battle,
    Showcase,
    Learning,
    Party,
    Professional,
    Themed
}

public struct VenueStaff {
    public let staffId: String;
    public let name: String;
    public let role: StaffRole;
    public let skillLevel: Int32;
    public let experience: Int32;
    public let shiftSchedule: array<WorkShift>;
    public let salary: Int32;
    public let customerRating: Float;
    public let specializations: array<String>;
    public let languages: array<String>;
}

public struct VenueAmenity {
    public let amenityId: String;
    public let amenityType: AmenityType;
    public let name: String;
    public let description: String;
    public let operationalCost: Int32;
    public let popularityBonus: Float;
    public let maintenanceRequirements: array<String>;
    public let upgradeOptions: array<String>;
}

public struct VIPArea {
    public let areaId: String;
    public let areaName: String;
    public let capacity: Int32;
    public let hourlyRate: Int32;
    public let amenities: array<VIPAmenity>;
    public let privacyLevel: PrivacyLevel;
    public let viewType: ViewType;
    public let serviceLevel: ServiceLevel;
    public let exclusivityRating: Float;
    public let reservationRequirements: array<String>;
}

public struct DressCode {
    public let codeId: String;
    public let codeName: String;
    public let requirements: array<ClothingRequirement>;
    public let restrictions: array<String>;
    public let exceptions: array<String>;
    public let enforcement: EnforcementLevel;
    public let violationPenalties: array<String>;
}

public struct OperatingSchedule {
    public let scheduleId: String;
    public let venueId: String;
    public let dailyHours: array<DailyHours>;
    public let specialHours: array<SpecialHours>;
    public let seasonalChanges: array<SeasonalSchedule>;
    public let eventSchedule: array<EventScheduleChange>;
}

public struct LicenseInfo {
    public let licenseId: String;
    public let licenseType: String;
    public let issuingAuthority: String;
    public let issueDate: Int64;
    public let expirationDate: Int64;
    public let restrictions: array<String>;
    public let renewalRequirements: array<String>;
    public let complianceStatus: String;
}

public struct Performer {
    public let performerId: String;
    public let performerName: String;
    public let performerType: PerformerType;
    public let genre: array<String>;
    public let skillLevel: Int32;
    public let popularity: Float;
    public let bookingFee: Int32;
    public let riderRequirements: array<String>;
    public let availability: array<String>;
    public let fanBase: Int32;
}

public struct DJ {
    public let djId: String;
    public let djName: String;
    public let musicStyles: array<MusicGenre>;
    public let skillRating: Float;
    public let equipment: array<DJEquipment>;
    public let setDuration: Int32;
    public let bookingFee: Int32;
    public let reputation: Float;
    public let fanFollowing: Int32;
    public let mixtapes: array<Mixtape>;
}

public struct VIPPackage {
    public let packageId: String;
    public let packageName: String;
    public let price: Int32;
    public let inclusions: array<PackageInclusion>;
    public let capacity: Int32;
    public let duration: Int32;
    public let exclusiveAccess: array<String>;
    public let personalServices: array<String>;
    public let upgrades: array<String>;
}

public struct EventSponsor {
    public let sponsorId: String;
    public let sponsorName: String;
    public let sponsorshipLevel: SponsorshipLevel;
    public let contribution: Int32;
    public let brandingRights: array<String>;
    public let promotionalRequirements: array<String>;
    public let audienceReach: Int32;
    public let brandValue: Int32;
}

public struct SocialMetrics {
    public let metricsId: String;
    public let eventId: String;
    public let socialMentions: Int32;
    public let hashtagUsage: Int32;
    public let photoShares: Int32;
    public let videoUploads: Int32;
    public let influencerEngagement: Int32;
    public let viralityScore: Float;
    public let sentimentAnalysis: SentimentScore;
}

public struct Bartender {
    public let bartenderId: String;
    public let name: String;
    public let skillLevel: Int32;
    public let specialties: array<DrinkCategory>;
    public let flairSkills: FlairLevel;
    public let customerServiceRating: Float;
    public let cocktailKnowledge: Int32;
    public let workShift: WorkShift;
    public let tips: Int32;
    public let regularCustomers: array<String>;
}

public struct CocktailRecipe {
    public let recipeId: String;
    public let cocktailName: String;
    public let ingredients: array<DrinkIngredient>;
    public let instructions: array<String>;
    public let difficulty: Int32;
    public let preparationTime: Int32;
    public let glassType: String;
    public let garnish: array<String>;
    public let price: Int32;
    public let popularity: Float;
}

public struct DrinkSpecialty {
    public let specialtyId: String;
    public let specialtyName: String;
    public let description: String;
    public let uniqueIngredients: array<String>;
    public let preparationMethod: String;
    public let presentationStyle: String;
    public let exclusivityLevel: ExclusivityLevel;
    public let seasonalAvailability: String;
}

public struct BarGame {
    public let gameId: String;
    public let gameName: String;
    public let gameType: GameType;
    public let playerCount: String;
    public let rules: array<String>;
    public let equipment: array<String>;
    public let skillRequired: Int32;
    public let averageGameTime: Int32;
    public let popularityRating: Float;
}

public struct SocialActivity {
    public let activityId: String;
    public let activityName: String;
    public let activityType: ActivityType;
    public let participantCount: String;
    public let duration: Int32;
    public let requirements: array<String>;
    public let socialBenefit: Float;
    public let funFactor: Float;
    public let networkingPotential: Float;
}

public struct LoyaltyProgram {
    public let programId: String;
    public let programName: String;
    public let venueId: String;
    public let memberBenefits: array<LoyaltyBenefit>;
    public let pointsSystem: PointsSystem;
    public let tierStructure: array<LoyaltyTier>;
    public let redemptionOptions: array<RedemptionOption>;
    public let memberCount: Int32;
    public let programValue: Int32;
}

public struct BarInventory {
    public let inventoryId: String;
    public let barId: String;
    public let alcoholStock: array<AlcoholItem>;
    public let mixerStock: array<MixerItem>;
    public let garnishStock: array<GarnishItem>;
    public let glassware: array<GlasswareItem>;
    public let equipment: array<BarEquipment>;
    public let reorderLevels: array<ReorderLevel>;
    public let monthlyConsumption: ConsumptionData;
}

public struct CustomerInteraction {
    public let interactionId: String;
    public let customerId: String;
    public let staffId: String;
    public let interactionType: String;
    public let timestamp: Int64;
    public let duration: Int32;
    public let satisfaction: Float;
    public let outcome: String;
    public let notes: String;
}

public struct CompetitionEvent {
    public let eventId: String;
    public let eventName: String;
    public let competitionType: CompetitionType;
    public let participants: array<String>;
    public let prizes: array<Prize>;
    public let rules: array<String>;
    public let judges: array<String>;
    public let schedule: CompetitionSchedule;
}

public struct LightingSystem {
    public let systemId: String;
    public let lightingType: LightingType;
    public let colorOptions: array<String>;
    public let effectsCapability: array<LightingEffect>;
    public let intensityControl: Bool;
    public let musicSync: Bool;
    public let programmableSequences: array<LightingSequence>;
    public let energyConsumption: Int32;
}

public struct SoundSystem {
    public let systemId: String;
    public let systemType: SoundSystemType;
    public let powerOutput: Int32;
    public let frequencyRange: String;
    public let speakerConfiguration: array<Speaker>;
    public let mixingConsole: MixingConsole;
    public let audioProcessing: array<AudioProcessor>;
    public let soundQuality: Float;
}

public struct MusicTrack {
    public let trackId: String;
    public let title: String;
    public let artist: String;
    public let genre: MusicGenre;
    public let duration: Int32;
    public let bpm: Int32;
    public let energy: Float;
    public let popularity: Float;
    public let danceability: Float;
    public let mood: MoodType;
}

public struct DancingPlayer {
    public let playerId: String;
    public let danceStyle: DanceStyle;
    public let skillLevel: Int32;
    public let energy: Float;
    public let joinTime: Int64;
    public let danceRating: Float;
    public let partnerId: String;
    public let performanceScore: Float;
    public let crowdAppeal: Float;
}

public struct DanceCompetition {
    public let competitionId: String;
    public let competitionName: String;
    public let danceStyle: DanceStyle;
    public let participants: array<CompetitionDancer>;
    public let judges: array<DanceJudge>;
    public let rounds: array<CompetitionRound>;
    public let prizes: array<DancePrize>;
    public let audienceVoting: Bool;
}

public struct SpecialEffect {
    public let effectId: String;
    public let effectName: String;
    public let effectType: EffectType;
    public let duration: Int32;
    public let intensity: Float;
    public let triggerConditions: array<String>;
    public let visualImpact: Float;
    public let audienceReaction: Float;
}

public struct LoungeAmenity {
    public let amenityId: String;
    public let amenityName: String;
    public let amenityType: String;
    public let description: String;
    public let exclusivity: Float;
    public let operatingCost: Int32;
    public let popularityBonus: Float;
}

public struct ReservationSystem {
    public let systemId: String;
    public let loungeId: String;
    public let bookingWindow: Int32;
    public let advanceBookingRequired: Bool;
    public let cancellationPolicy: CancellationPolicy;
    public let waitlistEnabled: Bool;
    public let priorityBooking: array<String>;
    public let automatedConfirmation: Bool;
}

public struct PersonalService {
    public let serviceId: String;
    public let serviceName: String;
    public let serviceType: PersonalServiceType;
    public let staffRequired: String;
    public let duration: Int32;
    public let cost: Int32;
    public let customization: array<String>;
}

public struct ExclusiveItem {
    public let itemId: String;
    public let itemName: String;
    public let itemType: String;
    public let description: String;
    public let price: Int32;
    public let availability: String;
    public let exclusivityLevel: Int32;
    public let preparation Time: Int32;
}

public struct GroupActivity {
    public let activityId: String;
    public let activityName: String;
    public let recommendedSize: String;
    public let duration: Int32;
    public let cost: Int32;
    public let venueRequirements: array<String>;
    public let socialBenefit: Float;
    public let funRating: Float;
}

public struct SocialDynamic {
    public let dynamicId: String;
    public let groupId: String;
    public let cohesionLevel: Float;
    public let leadershipStyle: String;
    public let conflictLevel: Float;
    public let communicationQuality: Float;
    public let sharedInterests: array<String>;
    public let groupMorale: Float;
}

public struct InteractionOutcome {
    public let outcomeId: String;
    public let outcomeType: String;
    public let description: String;
    public let positiveEffect: Float;
    public let negativeEffect: Float;
    public let duration: Int32;
    public let consequences: array<String>;
}

public struct RelationshipChange {
    public let changeId: String;
    public let participant1: String;
    public let participant2: String;
    public let relationshipType: String;
    public let changeValue: Float;
    public let changeReason: String;
    public let timestamp: Int64;
    public let permanence: Float;
}

public struct SocialGain {
    public let gainId: String;
    public let playerId: String;
    public let gainType: SocialGainType;
    public let gainAmount: Float;
    public let source: String;
    public let timestamp: Int64;
    public let multiplier: Float;
}

public enum StaffRole {
    Manager,
    Bartender,
    Server,
    Security,
    DJ,
    Host,
    Cleaner,
    Cook,
    Promoter,
    Technician,
    Bouncer,
    Valet,
    Coordinator,
    Artist,
    Entertainer
}

public enum AmenityType {
    Pool Table,
    Karaoke,
    Dance Floor,
    VIP Area,
    Outdoor Seating,
    Live Music Stage,
    Gaming Area,
    Food Service,
    Coat Check,
    Photography,
    Smoking Area,
    Private Rooms,
    Champagne Service,
    Cigar Lounge,
    Wine Cellar
}

public enum EnforcementLevel {
    Relaxed,
    Moderate,
    Strict,
    Zero Tolerance,
    Selective,
    Event Based,
    Time Based,
    VIP Exempt,
    Member Exempt,
    Situational
}

public enum PerformerType {
    Solo Artist,
    Band,
    DJ,
    Comedian,
    Dancer,
    Magician,
    Speaker,
    Host,
    Celebrity,
    Local Talent,
    Tribute Act,
    Cover Band,
    Original Act,
    Variety Act,
    Special Guest
}

public enum SponsorshipLevel {
    Title Sponsor,
    Presenting Sponsor,
    Major Sponsor,
    Supporting Sponsor,
    Media Partner,
    Venue Partner,
    Promotional Partner,
    Equipment Sponsor,
    Prize Sponsor,
    Community Partner
}

public enum FlairLevel {
    Basic,
    Intermediate,
    Advanced,
    Expert,
    Professional,
    Championship,
    Legendary,
    Master,
    Innovative,
    Signature
}

public enum GameType {
    Pool,
    Darts,
    Cards,
    Trivia,
    Arcade,
    Board Games,
    Video Games,
    Pinball,
    Foosball,
    Air Hockey,
    Shuffleboard,
    Beer Pong,
    Cornhole,
    Karaoke Battle,
    Dancing
}

public enum ActivityType {
    Mixer,
    Speed Networking,
    Group Games,
    Theme Nights,
    Contests,
    Workshops,
    Tastings,
    Live Entertainment,
    Dancing,
    Karaoke,
    Trivia,
    Sports Viewing,
    Art Shows,
    Fashion Shows,
    Charity Events
}

public enum CompetitionType {
    Bartending,
    Dancing,
    Singing,
    DJ Battle,
    Trivia,
    Pool Tournament,
    Darts Tournament,
    Karaoke Contest,
    Fashion Show,
    Talent Show,
    Comedy Contest,
    Art Competition,
    Mixology,
    Flair Bartending,
    Team Challenge
}

public enum LightingType {
    LED,
    Laser,
    Strobe,
    Moving Head,
    Color Wash,
    Spotlight,
    Blacklight,
    Intelligent,
    DMX Controlled,
    Sound Reactive,
    Programmable,
    Holographic,
    Projection,
    Neon,
    Fiber Optic
}

public enum SoundSystemType {
    Club System,
    Concert System,
    Background Music,
    High End Audio,
    Professional PA,
    Surround Sound,
    Vintage System,
    Wireless System,
    Smart System,
    Custom Built,
    Mobile System,
    Outdoor System,
    Intimate Setup,
    Immersive Audio,
    Premium System
}

public enum MoodType {
    Energetic,
    Relaxing,
    Romantic,
    Party,
    Chill,
    Intense,
    Mysterious,
    Uplifting,
    Melancholy,
    Aggressive,
    Dreamy,
    Nostalgic,
    Futuristic,
    Dark,
    Euphoric
}

public enum EffectType {
    Fog,
    Bubble,
    Pyrotechnics,
    Confetti,
    Snow,
    Wind,
    Scent,
    Temperature,
    Hologram,
    Projection,
    Laser Show,
    Light Show,
    Water Feature,
    Smoke Ring,
    Mirror Ball
}

public enum PersonalServiceType {
    Bottle Service,
    Personal Bartender,
    Dedicated Server,
    Personal Host,
    Photography,
    Massage,
    Styling,
    Transportation,
    Concierge,
    Security,
    Chef Service,
    Entertainment,
    Shopping,
    Planning,
    Cleanup
}

public enum SocialGainType {
    Reputation,
    Influence,
    Connections,
    Status,
    Respect,
    Trust,
    Popularity,
    Charisma,
    Leadership,
    Expertise,
    Recognition,
    Authority,
    Credibility,
    Network Size,
    Social Capital
}