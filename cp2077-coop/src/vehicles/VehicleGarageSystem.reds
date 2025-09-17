// Vehicle Garage and Modification System
// Phase 5.3: Comprehensive vehicle ownership, customization, and trading system

public struct VehicleGarage {
    public let garageId: String;
    public let ownerId: String;
    public let garageType: GarageType;
    public let location: String;
    public let capacity: Int32;
    public let securityLevel: Int32;
    public let vehicles: array<OwnedVehicle>;
    public let customizationBays: array<ModificationBay>;
    public let staff: array<GarageStaff>;
    public let equipment: array<GarageEquipment>;
    public let specializations: array<String>;
    public let monthlyUpkeep: Int32;
    public let insurancePolicies: array<InsurancePolicy>;
    public let accessPermissions: array<GarageAccess>;
}

public struct OwnedVehicle {
    public let vehicleId: String;
    public let ownerId: String;
    public let baseModel: String;
    public let customName: String;
    public let acquisitionDate: Int64;
    public let purchasePrice: Int32;
    public let currentValue: Int32;
    public let mileage: Int32;
    public let condition: VehicleCondition;
    public let modifications: array<VehicleModification>;
    public let performanceStats: VehicleStats;
    public let visualCustomizations: VisualCustomization;
    public let maintenanceHistory: array<MaintenanceRecord>;
    public let insuranceStatus: InsuranceStatus;
    public let storageLocation: String;
    public let sharingPermissions: array<SharingPermission>;
    public let competitionHistory: array<CompetitionRecord>;
}

public struct VehicleModification {
    public let modId: String;
    public let modType: ModificationType;
    public let partName: String;
    public let manufacturer: String;
    public let tier: ModificationTier;
    public let installationDate: Int64;
    public let cost: Int32;
    public let performanceImpact: PerformanceImpact;
    public let visualImpact: VisualImpact;
    public let durability: Float;
    public let warranty: WarrantyInfo;
    public let compatibility: array<String>;
    public let unlockRequirements: array<String>;
}

public struct VehicleCustomizationShop {
    public let shopId: String;
    public let shopName: String;
    public let location: String;
    public let specialization: array<ModificationType>;
    public let reputation: Float;
    public let priceMultiplier: Float;
    public let qualityMultiplier: Float;
    public let services: array<CustomizationService>;
    public let staff: array<MechanicInfo>;
    public let equipment: array<ShopEquipment>;
    public let inventory: array<ModificationPart>;
    public let customOrders: array<CustomOrder>;
}

public struct VehicleMarketplace {
    public let marketId: String;
    public let listings: array<VehicleListing>;
    public let auctions: array<VehicleAuction>;
    public let tradeRequests: array<TradeRequest>;
    public let dealerships: array<Dealership>;
    public let privateParties: array<PrivateSeller>;
    public let marketTrends: array<VehicleMarketTrend>;
    public let priceHistory: array<PriceHistory>;
    public let popularModifications: array<PopularMod>;
}

public struct VehicleInsurance {
    public let policyId: String;
    public let vehicleId: String;
    public let ownerId: String;
    public let insuranceType: InsuranceType;
    public let coverage: array<CoverageType>;
    public let monthlyPremium: Int32;
    public let deductible: Int32;
    public let policyValue: Int32;
    public let claims: array<InsuranceClaim>;
    public let riskAssessment: RiskAssessment;
    public let discounts: array<InsuranceDiscount>;
}

public enum GarageType {
    Personal,
    Crew,
    Commercial,
    Underground,
    Luxury,
    Industrial,
    Mobile,
    Temporary,
    Showcase,
    Racing
}

public enum VehicleCondition {
    Mint,
    Excellent,
    Good,
    Fair,
    Poor,
    Damaged,
    Salvage,
    Restored,
    Custom,
    Prototype
}

public enum ModificationType {
    Engine,
    Transmission,
    Suspension,
    Brakes,
    Tires,
    Turbo,
    Exhaust,
    Intake,
    ECU,
    Armor,
    Windows,
    Paint,
    Interior,
    Lighting,
    Sound,
    Weapons,
    Defense,
    Utility,
    Aesthetics,
    Electronics
}

public enum ModificationTier {
    Street,
    Sport,
    Track,
    Racing,
    Military,
    Corporate,
    Exotic,
    Prototype,
    Legendary,
    Custom
}

public class VehicleGarageSystem {
    private static let playerGarages: array<VehicleGarage>;
    private static let ownedVehicles: array<OwnedVehicle>;
    private static let customizationShops: array<VehicleCustomizationShop>;
    private static let vehicleMarketplace: VehicleMarketplace;
    private static let insurancePolicies: array<VehicleInsurance>;
    private static let modificationCatalog: array<ModificationCatalogItem>;
    
    public static func CreateGarage(ownerId: String, garageType: GarageType, location: String, capacity: Int32) -> String {
        if !CanAffordGarage(ownerId, garageType, location) {
            return "";
        }
        
        let garageId = "garage_" + ownerId + "_" + ToString(GetGameTime());
        let garage: VehicleGarage;
        garage.garageId = garageId;
        garage.ownerId = ownerId;
        garage.garageType = garageType;
        garage.location = location;
        garage.capacity = capacity;
        garage.securityLevel = GetBaseSecurityLevel(garageType);
        garage.customizationBays = InitializeModificationBays(garageType, capacity);
        garage.monthlyUpkeep = CalculateUpkeepCost(garageType, capacity, location);
        
        ArrayPush(playerGarages, garage);
        
        ProcessGaragePurchase(ownerId, garage);
        LogGarageEvent("GARAGE_CREATED", ownerId, garageId);
        
        return garageId;
    }
    
    public static func PurchaseVehicle(buyerId: String, vehicleModel: String, dealershipId: String, financing: FinancingOptions) -> String {
        if !CanAffordVehicle(buyerId, vehicleModel, financing) {
            return "";
        }
        
        let vehicleId = "vehicle_" + buyerId + "_" + ToString(GetGameTime());
        let vehicle: OwnedVehicle;
        vehicle.vehicleId = vehicleId;
        vehicle.ownerId = buyerId;
        vehicle.baseModel = vehicleModel;
        vehicle.customName = GetDefaultVehicleName(vehicleModel);
        vehicle.acquisitionDate = GetGameTime();
        vehicle.purchasePrice = GetVehiclePrice(vehicleModel, dealershipId);
        vehicle.currentValue = vehicle.purchasePrice;
        vehicle.condition = VehicleCondition.Mint;
        vehicle.performanceStats = GetBaseStats(vehicleModel);
        vehicle.visualCustomizations = GetDefaultVisuals(vehicleModel);
        
        ArrayPush(ownedVehicles, vehicle);
        
        ProcessVehiclePurchase(buyerId, vehicle, financing);
        AssignToGarage(vehicleId, GetDefaultGarage(buyerId));
        
        return vehicleId;
    }
    
    public static func ModifyVehicle(vehicleId: String, playerId: String, modifications: array<ModificationRequest>) -> Bool {
        let vehicle = GetOwnedVehicle(vehicleId);
        if !HasModificationRights(vehicle, playerId) {
            return false;
        }
        
        let totalCost = 0;
        let validMods: array<VehicleModification>;
        
        for modRequest in modifications {
            if ValidateModification(vehicle, modRequest) {
                let mod = CreateModification(modRequest);
                let cost = CalculateModificationCost(mod, vehicle);
                
                if CanAffordModification(playerId, cost) {
                    ArrayPush(validMods, mod);
                    totalCost += cost;
                }
            }
        }
        
        if ArraySize(validMods) == 0 {
            return false;
        }
        
        for mod in validMods {
            ApplyModification(vehicle, mod);
        }
        
        UpdateVehicleStats(vehicle);
        UpdateVehicleValue(vehicle);
        ProcessModificationPayment(playerId, totalCost);
        
        LogModificationEvent("VEHICLE_MODIFIED", playerId, vehicleId, ArraySize(validMods));
        return true;
    }
    
    public static func CustomizeVehicleAppearance(vehicleId: String, playerId: String, customization: AppearanceCustomization) -> Bool {
        let vehicle = GetOwnedVehicle(vehicleId);
        if !HasCustomizationRights(vehicle, playerId) {
            return false;
        }
        
        let cost = CalculateCustomizationCost(customization);
        if !CanAffordCustomization(playerId, cost) {
            return false;
        }
        
        ApplyVisualCustomization(vehicle, customization);
        ProcessCustomizationPayment(playerId, cost);
        
        UpdateVehiclePhotos(vehicle);
        LogCustomizationEvent("APPEARANCE_CUSTOMIZED", playerId, vehicleId);
        
        return true;
    }
    
    public static func ListVehicleForSale(vehicleId: String, sellerId: String, price: Int32, saleType: SaleType) -> String {
        let vehicle = GetOwnedVehicle(vehicleId);
        if !Equals(vehicle.ownerId, sellerId) {
            return "";
        }
        
        let listingId = "listing_" + vehicleId + "_" + ToString(GetGameTime());
        let listing: VehicleListing;
        listing.listingId = listingId;
        listing.vehicleId = vehicleId;
        listing.sellerId = sellerId;
        listing.askingPrice = price;
        listing.saleType = saleType;
        listing.listingDate = GetGameTime();
        listing.vehicleDetails = GenerateVehicleDetails(vehicle);
        listing.photos = GenerateListingPhotos(vehicle);
        listing.inspectionReports = GetVehicleInspection(vehicle);
        
        ArrayPush(vehicleMarketplace.listings, listing);
        
        NotifyInterestedBuyers(listing);
        UpdateMarketTrends(vehicle.baseModel);
        
        return listingId;
    }
    
    public static func ShareVehicleWithCrew(vehicleId: String, ownerId: String, crewId: String, permissions: SharingPermissions) -> Bool {
        let vehicle = GetOwnedVehicle(vehicleId);
        if !Equals(vehicle.ownerId, ownerId) {
            return false;
        }
        
        let sharing: SharingPermission;
        sharing.sharingId = vehicleId + "_" + crewId;
        sharing.vehicleId = vehicleId;
        sharing.sharedWithType = "CREW";
        sharing.sharedWithId = crewId;
        sharing.permissions = permissions;
        sharing.startDate = GetGameTime();
        sharing.restrictions = GenerateDefaultRestrictions();
        
        ArrayPush(vehicle.sharingPermissions, sharing);
        
        NotifyCrewMembers(crewId, "VEHICLE_SHARED", vehicleId);
        LogSharingEvent("VEHICLE_SHARED_WITH_CREW", ownerId, vehicleId, crewId);
        
        return true;
    }
    
    public static func OrganizeCarShow(organizerId: String, garageId: String, showType: CarShowType, categories: array<String>) -> String {
        let garage = GetGarage(garageId);
        if !HasEventPermissions(garage, organizerId) {
            return "";
        }
        
        let showId = "carshow_" + garageId + "_" + ToString(GetGameTime());
        let carShow: CarShow;
        carShow.showId = showId;
        carShow.organizerId = organizerId;
        carShow.garageId = garageId;
        carShow.showType = showType;
        carShow.categories = categories;
        carShow.startTime = GetGameTime() + 3600;
        carShow.registrationDeadline = carShow.startTime - 1800;
        carShow.prizes = GeneratePrizes(showType, categories);
        carShow.judgesCriteria = GetJudgingCriteria(showType);
        
        SendCarShowInvitations(carShow);
        PrepareShowVenue(garage);
        
        return showId;
    }
    
    public static func TuneVehicleForRacing(vehicleId: String, playerId: String, racingClass: RacingClass, trackType: TrackType) -> RacingSetup {
        let vehicle = GetOwnedVehicle(vehicleId);
        if !HasTuningRights(vehicle, playerId) {
            let empty: RacingSetup;
            return empty;
        }
        
        let setup: RacingSetup;
        setup.vehicleId = vehicleId;
        setup.racingClass = racingClass;
        setup.trackType = trackType;
        setup.engineTune = OptimizeEngineForRacing(vehicle, racingClass, trackType);
        setup.suspensionSetup = OptimizeSuspension(vehicle, trackType);
        setup.aerodynamics = OptimizeAerodynamics(vehicle, trackType);
        setup.gearing = OptimizeGearing(vehicle, trackType);
        setup.tireSetup = SelectOptimalTires(vehicle, trackType);
        setup.weightReduction = CalculateWeightSavings(vehicle);
        
        setup.estimatedPerformance = SimulateTrackPerformance(vehicle, setup);
        setup.competitiveness = AssessCompetitiveness(setup, racingClass);
        
        ApplyRacingSetup(vehicle, setup);
        return setup;
    }
    
    public static func GetVehicleInsurance(vehicleId: String, ownerId: String, coverageLevel: InsuranceLevel) -> String {
        let vehicle = GetOwnedVehicle(vehicleId);
        if !Equals(vehicle.ownerId, ownerId) {
            return "";
        }
        
        let policyId = "policy_" + vehicleId + "_" + ToString(GetGameTime());
        let policy: VehicleInsurance;
        policy.policyId = policyId;
        policy.vehicleId = vehicleId;
        policy.ownerId = ownerId;
        policy.insuranceType = DetermineInsuranceType(vehicle);
        policy.coverage = GetCoverageForLevel(coverageLevel);
        policy.policyValue = vehicle.currentValue;
        policy.riskAssessment = AssessInsuranceRisk(vehicle, ownerId);
        policy.monthlyPremium = CalculatePremium(vehicle, policy.riskAssessment, coverageLevel);
        policy.deductible = CalculateDeductible(vehicle.currentValue, coverageLevel);
        
        ArrayPush(insurancePolicies, policy);
        
        ProcessInsurancePayment(ownerId, policy.monthlyPremium);
        UpdateVehicleInsuranceStatus(vehicle, InsuranceStatus.Active);
        
        return policyId;
    }
    
    public static func ScheduleVehicleMaintenance(vehicleId: String, ownerId: String, maintenanceType: MaintenanceType) -> String {
        let vehicle = GetOwnedVehicle(vehicleId);
        if !Equals(vehicle.ownerId, ownerId) {
            return "";
        }
        
        let maintenanceId = "maintenance_" + vehicleId + "_" + ToString(GetGameTime());
        let maintenance: MaintenanceAppointment;
        maintenance.maintenanceId = maintenanceId;
        maintenance.vehicleId = vehicleId;
        maintenance.ownerId = ownerId;
        maintenance.maintenanceType = maintenanceType;
        maintenance.scheduledTime = GetGameTime() + 7200;
        maintenance.estimatedCost = CalculateMaintenanceCost(vehicle, maintenanceType);
        maintenance.estimatedDuration = CalculateMaintenanceDuration(vehicle, maintenanceType);
        maintenance.recommendedServices = GenerateServiceRecommendations(vehicle);
        
        ScheduleMaintenance(maintenance);
        NotifyOwner(ownerId, "MAINTENANCE_SCHEDULED", maintenanceId);
        
        return maintenanceId;
    }
    
    public static func TradeVehicles(playerId1: String, vehicleId1: String, playerId2: String, vehicleId2: String, cashDifference: Int32) -> String {
        let vehicle1 = GetOwnedVehicle(vehicleId1);
        let vehicle2 = GetOwnedVehicle(vehicleId2);
        
        if !Equals(vehicle1.ownerId, playerId1) || !Equals(vehicle2.ownerId, playerId2) {
            return "";
        }
        
        let tradeValue1 = CalculateTradeValue(vehicle1);
        let tradeValue2 = CalculateTradeValue(vehicle2);
        let requiredCash = tradeValue1 - tradeValue2;
        
        if Abs(requiredCash - cashDifference) > 1000 {
            return "";
        }
        
        let tradeId = "trade_" + vehicleId1 + "_" + vehicleId2;
        
        TransferVehicleOwnership(vehicleId1, playerId2);
        TransferVehicleOwnership(vehicleId2, playerId1);
        
        if cashDifference > 0 {
            TransferMoney(playerId2, playerId1, cashDifference);
        } else if cashDifference < 0 {
            TransferMoney(playerId1, playerId2, Abs(cashDifference));
        }
        
        LogTradeEvent("VEHICLE_TRADE", playerId1, playerId2, tradeId);
        
        return tradeId;
    }
    
    private static func UpdateVehicleStats(ref vehicle: OwnedVehicle) -> Void {
        let baseStats = GetBaseStats(vehicle.baseModel);
        let modifiedStats = baseStats;
        
        for mod in vehicle.modifications {
            ApplyModificationToStats(modifiedStats, mod);
        }
        
        vehicle.performanceStats = modifiedStats;
        vehicle.currentValue = CalculateModifiedVehicleValue(vehicle);
    }
    
    private static func ProcessMaintenanceSchedule() -> Void {
        let currentTime = GetGameTime();
        
        for vehicle in ownedVehicles {
            let maintenanceNeeds = AssessMaintenanceNeeds(vehicle, currentTime);
            
            for need in maintenanceNeeds {
                if need.urgent {
                    NotifyOwner(vehicle.ownerId, "URGENT_MAINTENANCE", need.description);
                } else if need.recommended {
                    NotifyOwner(vehicle.ownerId, "MAINTENANCE_REMINDER", need.description);
                }
            }
        }
    }
    
    private static func UpdateMarketValues() -> Void {
        for vehicle in ownedVehicles {
            let newValue = CalculateCurrentMarketValue(vehicle);
            vehicle.currentValue = newValue;
            
            NotifyOwnerOfValueChange(vehicle.ownerId, vehicle.vehicleId, newValue);
        }
    }
    
    public static func GetOwnedVehicle(vehicleId: String) -> OwnedVehicle {
        for vehicle in ownedVehicles {
            if Equals(vehicle.vehicleId, vehicleId) {
                return vehicle;
            }
        }
        
        let empty: OwnedVehicle;
        return empty;
    }
    
    public static func GetGarage(garageId: String) -> VehicleGarage {
        for garage in playerGarages {
            if Equals(garage.garageId, garageId) {
                return garage;
            }
        }
        
        let empty: VehicleGarage;
        return empty;
    }
    
    public static func InitializeVehicleSystem() -> Void {
        LoadVehicleCatalog();
        LoadModificationCatalog();
        InitializeCustomizationShops();
        LoadInsuranceProviders();
        StartMarketSimulation();
        ScheduleMaintenanceChecks();
        
        LogSystem("VehicleGarageSystem initialized successfully");
    }
}