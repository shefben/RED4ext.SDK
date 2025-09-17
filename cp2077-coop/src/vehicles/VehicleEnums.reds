// Vehicle System Enumerations and Supporting Types
// Supporting data structures for vehicle garage and modification system

public enum SaleType {
    FixedPrice,
    Auction,
    BestOffer,
    Trade,
    Consignment,
    Lease,
    Rental,
    Partnership
}

public enum InsuranceType {
    Basic,
    Comprehensive,
    Collision,
    Theft,
    Racing,
    Commercial,
    Exotic,
    Military
}

public enum CoverageType {
    Liability,
    Collision,
    Comprehensive,
    Theft,
    Vandalism,
    Racing,
    Modifications,
    TotalLoss,
    Rental,
    Roadside
}

public enum InsuranceStatus {
    Active,
    Expired,
    Suspended,
    Cancelled,
    PendingRenewal,
    Lapsed,
    UnderReview
}

public enum InsuranceLevel {
    Basic,
    Standard,
    Premium,
    Platinum,
    Racing,
    Commercial,
    Exotic
}

public enum MaintenanceType {
    Routine,
    Repair,
    Upgrade,
    Inspection,
    Cleaning,
    Storage,
    Restoration,
    Racing,
    Emergency
}

public enum CarShowType {
    Casual,
    Competition,
    Charity,
    Corporate,
    Racing,
    Vintage,
    Exotic,
    Custom,
    Themed,
    International
}

public enum RacingClass {
    Street,
    Sport,
    SuperCar,
    HyperCar,
    Touring,
    Formula,
    Rally,
    Drag,
    Drift,
    OpenWheel
}

public enum TrackType {
    Circuit,
    Drag,
    Rally,
    Street,
    Oval,
    Hill,
    Drift,
    Autocross,
    Endurance,
    Mixed
}

public struct VehicleStats {
    public let acceleration: Float;
    public let topSpeed: Float;
    public let handling: Float;
    public let braking: Float;
    public let durability: Float;
    public let weight: Float;
    public let power: Int32;
    public let torque: Int32;
    public let traction: Float;
    public let stability: Float;
}

public struct VisualCustomization {
    public let paintScheme: PaintScheme;
    public let bodykit: String;
    public let wheels: WheelConfiguration;
    public let lighting: LightingConfiguration;
    public let interior: InteriorConfiguration;
    public let decals: array<DecalConfiguration>;
    public let windowTint: WindowConfiguration;
    public let exhaust: ExhaustConfiguration;
}

public struct PaintScheme {
    public let schemeType: PaintType;
    public let primaryColor: String;
    public let secondaryColor: String;
    public let accentColor: String;
    public let finish: PaintFinish;
    public let pattern: String;
    public let pearlescence: Bool;
    public let metallic: Bool;
    public let customDesign: String;
}

public struct WheelConfiguration {
    public let wheelType: String;
    public let wheelSize: Int32;
    public let wheelColor: String;
    public let tireType: String;
    public let tireSize: String;
    public let rimMaterial: String;
    public let brakeCaliperColor: String;
}

public struct LightingConfiguration {
    public let headlightType: String;
    public let headlightColor: String;
    public let taillightType: String;
    public let underglow: Bool;
    public let underglowColor: String;
    public let interiorLighting: String;
    public let customLEDs: array<LEDConfiguration>;
}

public struct InteriorConfiguration {
    public let seatMaterial: String;
    public let seatColor: String;
    public let dashboardMaterial: String;
    public let steeringWheel: String;
    public let gearShifter: String;
    public let floorMats: String;
    public let customUpholstery: String;
}

public struct PerformanceImpact {
    public let accelerationChange: Float;
    public let topSpeedChange: Float;
    public let handlingChange: Float;
    public let brakingChange: Float;
    public let durabilityChange: Float;
    public let weightChange: Float;
    public let powerChange: Int32;
    public let fuelEfficiencyChange: Float;
}

public struct VisualImpact {
    public let appearanceRating: Float;
    public let uniquenessRating: Float;
    public let aggressionLevel: Float;
    public let luxuryLevel: Float;
    public let sportinessLevel: Float;
    public let streetCredBonus: Float;
}

public struct WarrantyInfo {
    public let warrantyType: String;
    public let duration: Int32;
    public let coverage: array<String>;
    public let terms: array<String>;
    public let provider: String;
    public let transferable: Bool;
}

public struct MaintenanceRecord {
    public let recordId: String;
    public let maintenanceDate: Int64;
    public let maintenanceType: MaintenanceType;
    public let serviceProvider: String;
    public let cost: Int32;
    public let description: String;
    public let partsReplaced: array<String>;
    public let nextServiceDue: Int64;
}

public struct SharingPermission {
    public let sharingId: String;
    public let vehicleId: String;
    public let sharedWithType: String;
    public let sharedWithId: String;
    public let permissions: SharingPermissions;
    public let startDate: Int64;
    public let endDate: Int64;
    public let restrictions: array<SharingRestriction>;
}

public struct SharingPermissions {
    public let canDrive: Bool;
    public let canModify: Bool;
    public let canRace: Bool;
    public let canLend: Bool;
    public let canSell: Bool;
    public let timeRestrictions: array<TimeRestriction>;
    public let locationRestrictions: array<String>;
}

public struct CompetitionRecord {
    public let recordId: String;
    public let eventId: String;
    public let eventType: String;
    public let eventDate: Int64;
    public let placement: Int32;
    public let totalParticipants: Int32;
    public let timeRecord: Float;
    public let prize: Int32;
    public let vehicleConfiguration: String;
}

public struct ModificationBay {
    public let bayId: String;
    public let bayType: ModificationBayType;
    public let equipment: array<String>;
    public let specializations: array<ModificationType>;
    public let currentProject: String;
    public let staffAssigned: String;
    public let occupancyStatus: Bool;
}

public struct GarageStaff {
    public let staffId: String;
    public let name: String;
    public let role: StaffRole;
    public let specializations: array<String>;
    public let skillLevel: Int32;
    public let experience: Int32;
    public let salary: Int32;
    public let availability: String;
}

public struct GarageEquipment {
    public let equipmentId: String;
    public let equipmentType: String;
    public let name: String;
    public let capabilities: array<String>;
    public let condition: String;
    public let maintenanceStatus: String;
    public let purchaseDate: Int64;
    public let warranty: String;
}

public struct InsurancePolicy {
    public let policyId: String;
    public let policyType: String;
    public let coverage: array<String>;
    public let deductible: Int32;
    public let premium: Int32;
    public let startDate: Int64;
    public let endDate: Int64;
}

public struct GarageAccess {
    public let accessId: String;
    public let grantedTo: String;
    public let accessLevel: AccessLevel;
    public let permissions: array<String>;
    public let timeRestrictions: array<TimeRestriction>;
    public let vehicleRestrictions: array<String>;
}

public struct CustomizationService {
    public let serviceId: String;
    public let serviceName: String;
    public let serviceType: String;
    public let basePrice: Int32;
    public let timeRequired: Int32;
    public let qualityLevel: Float;
    public let specializations: array<String>;
}

public struct MechanicInfo {
    public let mechanicId: String;
    public let name: String;
    public let specializations: array<String>;
    public let certifications: array<String>;
    public let experienceLevel: Int32;
    public let reputation: Float;
    public let hourlyRate: Int32;
}

public struct ShopEquipment {
    public let equipmentId: String;
    public let equipmentName: String;
    public let capabilities: array<String>;
    public let condition: String;
    public let lastMaintenance: Int64;
    public let efficiency: Float;
}

public struct ModificationPart {
    public let partId: String;
    public let partName: String;
    public let partType: ModificationType;
    public let manufacturer: String;
    public let tier: ModificationTier;
    public let price: Int32;
    public let availability: String;
    public let compatibility: array<String>;
}

public struct CustomOrder {
    public let orderId: String;
    public let customerId: String;
    public let vehicleId: String;
    public let specifications: array<String>;
    public let totalCost: Int32;
    public let estimatedCompletion: Int64;
    public let status: String;
}

public struct VehicleListing {
    public let listingId: String;
    public let vehicleId: String;
    public let sellerId: String;
    public let askingPrice: Int32;
    public let saleType: SaleType;
    public let listingDate: Int64;
    public let vehicleDetails: VehicleDetails;
    public let photos: array<String>;
    public let inspectionReports: array<InspectionReport>;
}

public struct VehicleAuction {
    public let auctionId: String;
    public let vehicleId: String;
    public let sellerId: String;
    public let startingBid: Int32;
    public let currentBid: Int32;
    public let reservePrice: Int32;
    public let auctionEndTime: Int64;
    public let bidders: array<AuctionBidder>;
    public let bidHistory: array<BidRecord>;
}

public struct TradeRequest {
    public let requestId: String;
    public let requesterId: String;
    public let offeredVehicle: String;
    public let requestedVehicle: String;
    public let cashDifference: Int32;
    public let message: String;
    public let expirationDate: Int64;
}

public struct Dealership {
    public let dealershipId: String;
    public let name: String;
    public let location: String;
    public let specialization: array<String>;
    public let inventory: array<VehicleInventoryItem>;
    public let services: array<String>;
    public let reputation: Float;
    public let priceMultiplier: Float;
}

public struct PrivateSeller {
    public let sellerId: String;
    public let name: String;
    public let reputation: Float;
    public let vehiclesForSale: array<String>;
    public let preferredPayment: array<String>;
    public let location: String;
    public let contactInfo: String;
}

public struct VehicleMarketTrend {
    public let trendId: String;
    public let vehicleModel: String;
    public let averagePrice: Int32;
    public let priceChange: Float;
    public let demandLevel: Float;
    public let supplyLevel: Float;
    public let popularModifications: array<String>;
}

public struct PriceHistory {
    public let vehicleModel: String;
    public let dateRange: String;
    public let pricePoints: array<PricePoint>;
    public let averagePrice: Int32;
    public let highestPrice: Int32;
    public let lowestPrice: Int32;
}

public struct PopularMod {
    public let modType: ModificationType;
    public let partName: String;
    public let popularityScore: Float;
    public let averageCost: Int32;
    public let compatibleVehicles: array<String>;
}

public struct InsuranceClaim {
    public let claimId: String;
    public let claimDate: Int64;
    public let claimType: String;
    public let claimAmount: Int32;
    public let status: String;
    public let description: String;
    public let resolution: String;
}

public struct RiskAssessment {
    public let riskScore: Float;
    public let factors: array<RiskFactor>;
    public let drivingHistory: DrivingHistory;
    public let vehicleRisk: VehicleRisk;
    public let locationRisk: LocationRisk;
    public let usageRisk: UsageRisk;
}

public struct InsuranceDiscount {
    public let discountType: String;
    public let discountAmount: Float;
    public let requirements: array<String>;
    public let expirationDate: Int64;
}

public struct ModificationRequest {
    public let requestId: String;
    public let modType: ModificationType;
    public let partId: String;
    public let specifications: array<String>;
    public let urgency: String;
    public let budget: Int32;
}

public struct AppearanceCustomization {
    public let customizationId: String;
    public let paintWork: PaintCustomization;
    public let bodyModifications: array<BodyModification>;
    public let interiorChanges: array<InteriorModification>;
    public let lightingChanges: array<LightingModification>;
    public let wheelChanges: WheelCustomization;
}

public struct FinancingOptions {
    public let financingType: String;
    public let downPayment: Int32;
    public let monthlyPayment: Int32;
    public let interestRate: Float;
    public let term: Int32;
    public let totalCost: Int32;
}

public struct RacingSetup {
    public let vehicleId: String;
    public let racingClass: RacingClass;
    public let trackType: TrackType;
    public let engineTune: EngineTuning;
    public let suspensionSetup: SuspensionTuning;
    public let aerodynamics: AerodynamicSetup;
    public let gearing: GearingSetup;
    public let tireSetup: TireSetup;
    public let weightReduction: WeightReduction;
    public let estimatedPerformance: PerformanceEstimate;
    public let competitiveness: CompetitivenessRating;
}

public struct CarShow {
    public let showId: String;
    public let organizerId: String;
    public let garageId: String;
    public let showType: CarShowType;
    public let categories: array<String>;
    public let startTime: Int64;
    public let registrationDeadline: Int64;
    public let prizes: array<Prize>;
    public let judgesCriteria: array<JudgingCriteria>;
}

public struct MaintenanceAppointment {
    public let maintenanceId: String;
    public let vehicleId: String;
    public let ownerId: String;
    public let maintenanceType: MaintenanceType;
    public let scheduledTime: Int64;
    public let estimatedCost: Int32;
    public let estimatedDuration: Int32;
    public let recommendedServices: array<RecommendedService>;
}

public enum ModificationBayType {
    Basic,
    Professional,
    Racing,
    Exotic,
    Restoration,
    Paint,
    Engine,
    Electronics
}

public enum StaffRole {
    Mechanic,
    Tuner,
    Painter,
    Detailer,
    Inspector,
    Manager,
    Specialist,
    Apprentice
}

public enum PaintType {
    Solid,
    Metallic,
    Pearlescent,
    Matte,
    Satin,
    Chrome,
    Chameleon,
    Custom
}

public enum PaintFinish {
    Gloss,
    SemiGloss,
    Satin,
    Matte,
    Flat,
    Textured,
    Metallic,
    Pearl
}

public struct SharingRestriction {
    public let restrictionType: String;
    public let restrictionValue: String;
    public let description: String;
}

public struct VehicleDetails {
    public let make: String;
    public let model: String;
    public let year: Int32;
    public let mileage: Int32;
    public let condition: VehicleCondition;
    public let modifications: array<String>;
    public let maintenanceHistory: String;
    public let accidents: array<String>;
}

public struct InspectionReport {
    public let reportId: String;
    public let inspectionDate: Int64;
    public let inspector: String;
    public let overallRating: Float;
    public let findings: array<String>;
    public let recommendations: array<String>;
}

public struct AuctionBidder {
    public let bidderId: String;
    public let currentBid: Int32;
    public let maxBid: Int32;
    public let bidTime: Int64;
    public let verified: Bool;
}

public struct BidRecord {
    public let bidId: String;
    public let bidderId: String;
    public let bidAmount: Int32;
    public let bidTime: Int64;
    public let bidType: String;
}

public struct VehicleInventoryItem {
    public let inventoryId: String;
    public let vehicleModel: String;
    public let price: Int32;
    public let availability: String;
    public let features: array<String>;
    public let options: array<String>;
}

public struct PricePoint {
    public let date: Int64;
    public let price: Int32;
    public let volume: Int32;
    public let marketCondition: String;
}

public struct RiskFactor {
    public let factorType: String;
    public let impact: Float;
    public let description: String;
}

public struct DrivingHistory {
    public let totalMiles: Int32;
    public let accidents: Int32;
    public let violations: Int32;
    public let claimsHistory: Int32;
    public let experienceYears: Int32;
}

public struct VehicleRisk {
    public let theftRisk: Float;
    public let accidentRisk: Float;
    public let damageRisk: Float;
    public let valueRisk: Float;
}

public struct LocationRisk {
    public let crimeRate: Float;
    public let weatherRisk: Float;
    public let trafficRisk: Float;
    public let parkingRisk: Float;
}

public struct UsageRisk {
    public let dailyMiles: Int32;
    public let usageType: String;
    public let riskActivities: array<String>;
    public let storageType: String;
}