// Housing System Enumerations and Supporting Types
// Supporting enums and data structures for apartment customization system

public enum FurnitureType {
    Sofa,
    Chair,
    Table,
    Bed,
    Desk,
    Cabinet,
    Shelf,
    Television,
    Computer,
    Refrigerator,
    Stove,
    Washer,
    Dryer,
    Bar,
    Pool,
    Shower,
    Bathtub,
    Mirror,
    Artwork,
    Plant,
    Lighting,
    Sound,
    Security,
    Storage,
    Workout
}

public enum ItemRarity {
    Common,
    Uncommon,
    Rare,
    Epic,
    Legendary,
    Iconic,
    Custom
}

public enum BaseSize {
    Small,
    Medium,
    Large,
    Massive,
    Complex
}

public enum VisitType {
    Social,
    Business,
    Romantic,
    Party,
    Meeting,
    Emergency,
    Maintenance,
    Inspection
}

public enum PartyType {
    Casual,
    Birthday,
    Corporate,
    Celebration,
    Networking,
    Launch,
    Memorial,
    Holiday
}

public enum TourType {
    Standard,
    Luxury,
    Business,
    Social,
    Investment,
    Rental
}

public enum InvestmentType {
    Buy,
    Rent,
    Flip,
    Development,
    Commercial,
    Portfolio
}

public enum BusinessMeetingType {
    Planning,
    Strategy,
    Review,
    Negotiation,
    Training,
    Briefing,
    Celebration,
    Crisis
}

public struct DecorationItem {
    public let itemId: String;
    public let decorationType: DecorationType;
    public let name: String;
    public let position: Vector3;
    public let value: Int32;
    public let theme: String;
    public let rarity: ItemRarity;
}

public struct ElectronicDevice {
    public let deviceId: String;
    public let deviceType: ElectronicType;
    public let brand: String;
    public let model: String;
    public let functionality: array<String>;
    public let networkConnected: Bool;
    public let powerConsumption: Int32;
    public let value: Int32;
}

public struct ApartmentAmenity {
    public let amenityId: String;
    public let amenityType: AmenityType;
    public let description: String;
    public let monthlyFee: Int32;
    public let availability: String;
    public let qualityRating: Float;
}

public struct AccessPermission {
    public let permissionId: String;
    public let grantedTo: String;
    public let permissionLevel: PermissionLevel;
    public let areas: array<String>;
    public let timeRestrictions: array<TimeRestriction>;
    public let expirationDate: Int64;
}

public struct VisitRecord {
    public let visitId: String;
    public let visitorId: String;
    public let visitDate: Int64;
    public let duration: Int32;
    public let purpose: String;
    public let rating: Int32;
}

public struct RoomModifier {
    public let modifierId: String;
    public let modifierType: ModifierType;
    public let effect: String;
    public let magnitude: Float;
    public let duration: Int32;
}

public struct ColorScheme {
    public let schemeName: String;
    public let primaryColor: String;
    public let secondaryColor: String;
    public let accentColor: String;
    public let lighting: String;
    public let mood: String;
}

public struct BaseFacility {
    public let facilityId: String;
    public let facilityType: FacilityType;
    public let capacity: Int32;
    public let operationalLevel: Int32;
    public let maintenanceStatus: String;
    public let specialFeatures: array<String>;
}

public struct OperationalSystem {
    public let systemId: String;
    public let systemType: SystemType;
    public let status: String;
    public let efficiency: Float;
    public let lastMaintenance: Int64;
    public let upgradeLevel: Int32;
}

public struct BaseMemberAccess {
    public let memberId: String;
    public let accessLevel: AccessLevel;
    public let areas: array<String>;
    public let permissions: array<String>;
    public let schedule: array<TimeSlot>;
}

public struct PropertyListing {
    public let listingId: String;
    public let propertyId: String;
    public let price: Int32;
    public let listingType: ListingType;
    public let description: String;
    public let features: array<String>;
    public let photos: array<String>;
    public let contact: String;
    public let availability: String;
}

public struct SaleRecord {
    public let saleId: String;
    public let propertyId: String;
    public let sellerId: String;
    public let buyerId: String;
    public let salePrice: Int32;
    public let saleDate: Int64;
    public let propertyType: ApartmentType;
    public let location: String;
}

public struct MarketTrend {
    public let trendId: String;
    public let location: String;
    public let propertyType: ApartmentType;
    public let averagePrice: Int32;
    public let priceChange: Float;
    public let demandLevel: Int32;
    public let supplyLevel: Int32;
    public let forecast: String;
}

public struct RentalListing {
    public let listingId: String;
    public let propertyId: String;
    public let monthlyRent: Int32;
    public let deposit: Int32;
    public let leaseTerm: Int32;
    public let availability: String;
    public let requirements: array<String>;
}

public struct Investment {
    public let investmentId: String;
    public let investorId: String;
    public let propertyIds: array<String>;
    public let totalInvestment: Int32;
    public let currentValue: Int32;
    public let expectedReturn: Float;
    public let strategy: String;
}

public struct DevelopmentProject {
    public let projectId: String;
    public let developerId: String;
    public let location: String;
    public let projectType: ProjectType;
    public let estimatedCost: Int32;
    public let completionDate: Int64;
    public let unitsAvailable: Int32;
    public let features: array<String>;
}

public struct CustomizationItem {
    public let itemId: String;
    public let category: String;
    public let name: String;
    public let description: String;
    public let price: Int32;
    public let rarity: ItemRarity;
    public let compatibility: array<String>;
    public let unlockRequirements: array<String>;
}

public struct DecoratorService {
    public let serviceId: String;
    public let decoratorName: String;
    public let specialties: array<String>;
    public let rating: Float;
    public let priceRange: String;
    public let availability: String;
    public let portfolio: array<String>;
}

public struct RealEstateAgent {
    public let agentId: String;
    public let name: String;
    public let agency: String;
    public let specialization: array<String>;
    public let rating: Float;
    public let commission: Float;
    public let contacts: array<String>;
}

public struct ApartmentVisit {
    public let visitId: String;
    public let hostId: String;
    public let guestId: String;
    public let propertyId: String;
    public let visitType: VisitType;
    public let startTime: Int64;
    public let maxDuration: Int32;
    public let permissions: array<String>;
}

public struct HouseParty {
    public let partyId: String;
    public let hostId: String;
    public let propertyId: String;
    public let partyType: PartyType;
    public let maxGuests: Int32;
    public let startTime: Int64;
    public let activities: array<String>;
    public let catering: String;
}

public struct DecorationProject {
    public let projectId: String;
    public let propertyId: String;
    public let decoratorId: String;
    public let clientId: String;
    public let budget: Int32;
    public let styleRequirements: String;
    public let timeline: Int32;
}

public struct VirtualTour {
    public let tourId: String;
    public let propertyId: String;
    public let creatorId: String;
    public let tourType: TourType;
    public let roomSequence: array<String>;
    public let highlights: array<String>;
    public let narration: String;
}

public struct PropertyInvestment {
    public let investmentId: String;
    public let investorId: String;
    public let investmentType: InvestmentType;
    public let properties: array<String>;
    public let strategy: String;
    public let initialValue: Int32;
    public let expectedReturn: Float;
}

public struct BusinessMeeting {
    public let meetingId: String;
    public let hostId: String;
    public let baseId: String;
    public let meetingType: BusinessMeetingType;
    public let attendees: array<String>;
    public let agenda: array<String>;
    public let scheduledTime: Int64;
    public let conferenceRoom: String;
}

public struct SearchCriteria {
    public let maxPrice: Int32;
    public let minRooms: Int32;
    public let district: String;
    public let amenities: array<String>;
    public let propertyType: ApartmentType;
    public let securityLevel: Int32;
}

public struct CustomizationChange {
    public let changeId: String;
    public let changeType: String;
    public let targetItem: String;
    public let newValue: String;
    public let cost: Int32;
}

public struct TimeRestriction {
    public let dayOfWeek: Int32;
    public let startHour: Int32;
    public let endHour: Int32;
    public let allowAccess: Bool;
}

public struct TimeSlot {
    public let startTime: Int64;
    public let endTime: Int64;
    public let recurring: Bool;
    public let description: String;
}

public enum DecorationType {
    Painting,
    Sculpture,
    Poster,
    Photography,
    Neon,
    Hologram,
    Plant,
    Vase,
    Candle,
    Clock,
    Mirror,
    Rug,
    Curtain,
    Pillow
}

public enum ElectronicType {
    Television,
    Computer,
    Gaming,
    Audio,
    Security,
    Climate,
    Lighting,
    Kitchen,
    Cleaning,
    Communication,
    Entertainment,
    Fitness,
    Medical,
    Netrunning
}

public enum AmenityType {
    Gym,
    Pool,
    Parking,
    Concierge,
    Security,
    Laundry,
    Storage,
    Rooftop,
    Garden,
    BusinessCenter,
    Restaurant,
    Spa,
    Garage,
    Medical
}

public enum PermissionLevel {
    Guest,
    Roommate,
    Partner,
    Family,
    Business,
    Staff,
    Owner,
    Admin
}

public enum ModifierType {
    Comfort,
    Productivity,
    Security,
    Entertainment,
    Health,
    Social,
    Skill,
    Recovery,
    Mood,
    Energy
}

public enum FacilityType {
    Armory,
    Workshop,
    MedBay,
    ServerRoom,
    Communications,
    Planning,
    Storage,
    Garage,
    Kitchen,
    Recreation,
    Training,
    Laboratory,
    Security,
    Meeting
}

public enum SystemType {
    Security,
    Communications,
    Power,
    Climate,
    Network,
    Transportation,
    Storage,
    Defense,
    Surveillance,
    Automation
}

public enum AccessLevel {
    Restricted,
    Basic,
    Standard,
    Advanced,
    Executive,
    Admin
}

public enum ListingType {
    Sale,
    Rent,
    Lease,
    Share,
    Exchange,
    Auction
}

public enum ProjectType {
    Residential,
    Commercial,
    Mixed,
    Luxury,
    Affordable,
    Corporate,
    Industrial,
    Entertainment
}