// Player economy system with trading, auction house, and dynamic markets

public enum TradeOfferType {
    Item = 0,
    Service = 1,
    Currency = 2,
    Information = 3,
    Vehicle = 4,
    Property = 5,
    Favor = 6
}

public enum MarketType {
    Legal = 0,           // Normal shops and vendors
    Gray = 1,            // Questionable legality
    Black = 2,           // Illegal markets
    Corporate = 3,       // Corporate exclusive
    Underground = 4,     // Hidden/invite-only
    Auction = 5         // Auction house
}

public enum TransactionStatus {
    Pending = 0,
    Active = 1,
    Completed = 2,
    Cancelled = 3,
    Disputed = 4,
    Expired = 5
}

public struct TradeOffer {
    public var offerId: String;
    public var sellerId: String;
    public var buyerId: String;
    public var offerType: TradeOfferType;
    public var title: String;
    public var description: String;
    public var askingPrice: Int32;
    public var currency: String; // "eddies", "favor_tokens", "data_shards"
    public var items: array<TradableItem>;
    public var services: array<TradableService>;
    public var location: String; // Where trade should happen
    public var marketType: MarketType;
    public var duration: Float; // How long offer is valid
    public var createdTime: Float;
    public var status: TransactionStatus;
    public var requirements: array<String>; // Buyer requirements
    public var isNegotiable: Bool;
    public var reputation: Int32; // Seller's reputation
    public var escrowEnabled: Bool; // Use secure escrow
}

public struct TradableItem {
    public var itemId: String;
    public var itemName: String;
    public var category: String; // "weapon", "armor", "cyberware", "consumable", "crafting"
    public var quality: String; // "common", "uncommon", "rare", "epic", "legendary"
    public var quantity: Int32;
    public var condition: Float; // 0-100% condition
    public var modifications: array<String>;
    public var marketValue: Int32; // Estimated value
    public var legality: String; // "legal", "restricted", "illegal"
    public var description: String;
    public var serialNumber: String; // For tracking stolen goods
}

public struct TradableService {
    public var serviceId: String;
    public var serviceName: String;
    public var category: String; // "crafting", "hacking", "transport", "protection", "information"
    public var description: String;
    public var duration: Float; // How long service takes
    public var deliveryTime: Float; // When service will be completed
    public var guarantees: array<String>; // Service guarantees
    public var requirements: array<String>; // What service provider needs
    public var reputation: Int32; // Service provider reputation
}

public struct AuctionListing {
    public var auctionId: String;
    public var sellerId: String;
    public var item: TradableItem;
    public var startingBid: Int32;
    public var currentBid: Int32;
    public var highestBidderId: String;
    public var reservePrice: Int32; // Minimum sale price
    public var bidIncrement: Int32; // Minimum bid increase
    public var endTime: Float;
    public var bidHistory: array<AuctionBid>;
    public var watchers: array<String>; // Players watching auction
    public var isActive: Bool;
    public var category: String;
    public var featuredListing: Bool; // Highlighted listing
}

public struct AuctionBid {
    public var bidderId: String;
    public var bidAmount: Int32;
    public var bidTime: Float;
    public var isProxy: Bool; // Automatic proxy bid
    public var maxProxyAmount: Int32; // Maximum auto-bid
}

public struct MarketData {
    public var itemId: String;
    public var averagePrice: Int32;
    public var priceHistory: array<PricePoint>;
    public var dailyVolume: Int32; // Items traded today
    public var weeklyVolume: Int32;
    public var demand: Float; // 0-1 demand level
    public var supply: Float; // 0-1 supply level
    public var priceVolatility: Float; // Price stability
    public var marketTrend: String; // "rising", "falling", "stable"
}

public struct PricePoint {
    public var price: Int32;
    public var timestamp: Float;
    public var volume: Int32; // How many items at this price
}

public struct PlayerWallet {
    public var playerId: String;
    public var eddies: Int32;
    public var favorTokens: Int32; // Earned through reputation
    public var dataShards: Int32; // Information currency
    public var corporateCredits: array<CorporateCredit>;
    public var cryptoCurrency: Int32; // Underground currency
    public var tradeReputation: Int32; // Trading reputation
    public var blackMarketAccess: Bool;
    public var escrowBalance: Int32; // Money in escrow
    public var creditLimit: Int32; // How much they can borrow
}

public struct CorporateCredit {
    public var corporation: String;
    public var amount: Int32;
    public var restrictions: array<String>; // What can be bought
}

public struct ShopListing {
    public var shopId: String;
    public var ownerId: String;
    public var shopName: String;
    public var location: Vector3;
    public var marketType: MarketType;
    public var specialization: array<String>; // What types of items sold
    public var inventory: array<TradableItem>;
    public var services: array<TradableService>;
    public var reputation: Int32;
    public var openHours: String; // When shop is open
    public var isPlayerOwned: Bool;
    public var securityLevel: Int32; // Protection against theft
    public var dailyRevenue: Int32;
    public var monthlyRevenue: Int32;
}

public struct EconomicEvent {
    public var eventId: String;
    public var eventType: String; // "shortage", "surplus", "corporate_raid", "gang_war"
    public var affectedItems: array<String>;
    public var priceMultiplier: Float; // How much prices are affected
    public var duration: Float; // How long event lasts
    public var description: String;
    public var startTime: Float;
    public var severity: Int32; // 1-10 impact level
}

public class PlayerEconomy {
    private static var isInitialized: Bool = false;
    private static var activeOffers: array<TradeOffer>;
    private static var auctionListings: array<AuctionListing>;
    private static var playerWallets: array<PlayerWallet>;
    private static var playerShops: array<ShopListing>;
    private static var marketData: array<MarketData>;
    private static var economicEvents: array<EconomicEvent>;
    private static var economyUI: ref<PlayerEconomyUI>;
    private static var updateInterval: Float = 60.0; // Update every minute
    private static var lastUpdateTime: Float = 0.0;
    
    // Network callbacks
    private static cb func OnTradeOfferCreated(data: String) -> Void;
    private static cb func OnTradeOfferAccepted(data: String) -> Void;
    private static cb func OnAuctionBid(data: String) -> Void;
    private static cb func OnMarketTransaction(data: String) -> Void;
    private static cb func OnShopUpdate(data: String) -> Void;
    
    public static func Initialize() -> Void {
        if isInitialized {
            return;
        }
        
        LogChannel(n"COOP_ECONOMY", "Initializing player economy system...");
        
        // Initialize player wallets
        PlayerEconomy.InitializePlayerWallets();
        
        // Initialize market data
        PlayerEconomy.InitializeMarketData();
        
        // Generate initial economic events
        PlayerEconomy.GenerateEconomicEvents();
        
        // Register network callbacks
        NetworkingSystem.RegisterCallback("trade_offer_created", PlayerEconomy.OnTradeOfferCreated);
        NetworkingSystem.RegisterCallback("trade_offer_accepted", PlayerEconomy.OnTradeOfferAccepted);
        NetworkingSystem.RegisterCallback("auction_bid", PlayerEconomy.OnAuctionBid);
        NetworkingSystem.RegisterCallback("market_transaction", PlayerEconomy.OnMarketTransaction);
        NetworkingSystem.RegisterCallback("shop_update", PlayerEconomy.OnShopUpdate);
        
        // Start update loop
        PlayerEconomy.StartUpdateLoop();
        
        isInitialized = true;
        LogChannel(n"COOP_ECONOMY", "Player economy system initialized");
    }
    
    private static func InitializePlayerWallets() -> Void {
        ArrayClear(playerWallets);
        
        let connectedPlayers = NetworkingSystem.GetConnectedPlayerIds();
        for playerId in connectedPlayers {
            let wallet: PlayerWallet;
            wallet.playerId = playerId;
            wallet.eddies = 50000; // Starting money
            wallet.favorTokens = 10;
            wallet.dataShards = 5;
            wallet.cryptoCurrency = 0;
            wallet.tradeReputation = 100; // Neutral starting reputation
            wallet.blackMarketAccess = false;
            wallet.escrowBalance = 0;
            wallet.creditLimit = 10000; // Can borrow up to 10k
            
            ArrayPush(playerWallets, wallet);
        }
    }
    
    private static func InitializeMarketData() -> Void {
        ArrayClear(marketData);
        
        // Common item categories and base prices
        let itemCategories = [
            "pistol_common", "rifle_common", "shotgun_common",
            "armor_light", "armor_medium", "armor_heavy",
            "cybereye_basic", "cybereye_advanced", "cyberarm_basic",
            "food_basic", "alcohol", "drugs_legal", "drugs_illegal",
            "crafting_common", "crafting_rare", "crafting_legendary",
            "vehicle_economy", "vehicle_sport", "vehicle_luxury",
            "data_corporate", "data_personal", "data_classified"
        ];
        
        let basePrices = [
            500, 1500, 800,
            300, 800, 2000,
            1000, 5000, 3000,
            10, 50, 100, 500,
            25, 100, 1000,
            15000, 50000, 200000,
            100, 50, 1000
        ];
        
        for i in Range(ArraySize(itemCategories)) {
            let market: MarketData;
            market.itemId = itemCategories[i];
            market.averagePrice = basePrices[i];
            market.dailyVolume = RandRange(5, 50);
            market.weeklyVolume = market.dailyVolume * 7;
            market.demand = RandRangeF(0.3, 0.8);
            market.supply = RandRangeF(0.3, 0.8);
            market.priceVolatility = RandRangeF(0.1, 0.4);
            market.marketTrend = PlayerEconomy.DetermineMarketTrend(market.demand, market.supply);
            
            // Initialize price history
            PlayerEconomy.GeneratePriceHistory(market);
            
            ArrayPush(marketData, market);
        }
    }
    
    private static func GeneratePriceHistory(market: ref<MarketData>) -> Void {
        let basePrice = market.averagePrice;
        let currentTime = GetGameTime();
        
        // Generate 7 days of price history
        for i in Range(7) {
            let dayOffset = Cast<Float>(i - 7) * 86400.0; // 24 hours in seconds
            let priceVariation = RandRangeF(-0.15, 0.15) * Cast<Float>(basePrice);
            
            let pricePoint: PricePoint;
            pricePoint.price = Cast<Int32>(Cast<Float>(basePrice) + priceVariation);
            pricePoint.timestamp = currentTime + dayOffset;
            pricePoint.volume = RandRange(1, 20);
            
            ArrayPush(market.priceHistory, pricePoint);
        }
    }
    
    private static func DetermineMarketTrend(demand: Float, supply: Float) -> String {
        let demandSupplyRatio = demand / supply;
        
        if demandSupplyRatio > 1.2 {
            return "rising";
        } else if demandSupplyRatio < 0.8 {
            return "falling";
        } else {
            return "stable";
        }
    }
    
    private static func GenerateEconomicEvents() -> Void {
        ArrayClear(economicEvents);
        
        // Generate some initial economic events
        PlayerEconomy.CreateEconomicEvent("weapon_shortage", "Gang War Impact", 
            ["pistol_common", "rifle_common"], 1.5, 3600.0 * 24, 6); // 24 hours, medium impact
        
        PlayerEconomy.CreateEconomicEvent("cyberware_surplus", "Corporate Overproduction",
            ["cybereye_basic", "cyberarm_basic"], 0.7, 3600.0 * 48, 4); // 48 hours, low impact
        
        PlayerEconomy.CreateEconomicEvent("data_leak", "Massive Data Breach",
            ["data_corporate", "data_classified"], 0.3, 3600.0 * 12, 8); // 12 hours, high impact
    }
    
    private static func CreateEconomicEvent(id: String, description: String, items: array<String>, multiplier: Float, duration: Float, severity: Int32) -> Void {
        let event: EconomicEvent;
        event.eventId = id + "_" + ToString(GetGameTime());
        event.eventType = id;
        event.affectedItems = items;
        event.priceMultiplier = multiplier;
        event.duration = duration;
        event.description = description;
        event.startTime = GetGameTime();
        event.severity = severity;
        
        ArrayPush(economicEvents, event);
        
        LogChannel(n"COOP_ECONOMY", "Created economic event: " + description);
    }
    
    public static func CreateTradeOffer(sellerId: String, offerType: TradeOfferType, title: String, description: String, 
                                       askingPrice: Int32, currency: String, items: array<TradableItem>, 
                                       location: String, marketType: MarketType, duration: Float) -> String {
        let offerId = "offer_" + sellerId + "_" + ToString(GetGameTime());
        
        let offer: TradeOffer;
        offer.offerId = offerId;
        offer.sellerId = sellerId;
        offer.buyerId = ""; // Will be set when someone accepts
        offer.offerType = offerType;
        offer.title = title;
        offer.description = description;
        offer.askingPrice = askingPrice;
        offer.currency = currency;
        offer.items = items;
        offer.location = location;
        offer.marketType = marketType;
        offer.duration = duration;
        offer.createdTime = GetGameTime();
        offer.status = TransactionStatus.Active;
        offer.isNegotiable = true;
        offer.escrowEnabled = askingPrice >= 10000; // Escrow for high-value trades
        
        // Get seller reputation
        let sellerWallet = PlayerEconomy.GetPlayerWallet(sellerId);
        offer.reputation = sellerWallet.tradeReputation;
        
        ArrayPush(activeOffers, offer);
        
        LogChannel(n"COOP_ECONOMY", "Created trade offer: " + offerId + " by " + sellerId);
        
        // Update market data
        for item in items {
            PlayerEconomy.UpdateMarketSupply(item.category, item.quantity);
        }
        
        // Broadcast offer
        let offerData = PlayerEconomy.SerializeTradeOffer(offer);
        NetworkingSystem.BroadcastMessage("trade_offer_created", offerData);
        
        return offerId;
    }
    
    public static func AcceptTradeOffer(offerId: String, buyerId: String) -> Bool {
        let offerIndex = PlayerEconomy.FindOfferIndex(offerId);
        if offerIndex == -1 {
            LogChannel(n"COOP_ECONOMY", "Trade offer not found: " + offerId);
            return false;
        }
        
        let offer = activeOffers[offerIndex];
        
        if offer.status != TransactionStatus.Active {
            LogChannel(n"COOP_ECONOMY", "Trade offer not active");
            return false;
        }
        
        if Equals(offer.sellerId, buyerId) {
            LogChannel(n"COOP_ECONOMY", "Cannot accept own trade offer");
            return false;
        }
        
        // Check if buyer can afford it
        let buyerWallet = PlayerEconomy.GetPlayerWallet(buyerId);
        let affordability = PlayerEconomy.CanAfford(buyerWallet, offer.askingPrice, offer.currency);
        
        if !affordability {
            LogChannel(n"COOP_ECONOMY", "Buyer cannot afford trade offer");
            return false;
        }
        
        // Process transaction
        if offer.escrowEnabled {
            return PlayerEconomy.ProcessEscrowTransaction(offerIndex, buyerId);
        } else {
            return PlayerEconomy.ProcessDirectTransaction(offerIndex, buyerId);
        }
    }
    
    private static func ProcessDirectTransaction(offerIndex: Int32, buyerId: String) -> Bool {
        let offer = activeOffers[offerIndex];
        
        // Deduct money from buyer
        let buyerWallet = PlayerEconomy.GetPlayerWallet(buyerId);
        PlayerEconomy.DeductCurrency(buyerWallet, offer.askingPrice, offer.currency);
        
        // Add money to seller
        let sellerWallet = PlayerEconomy.GetPlayerWallet(offer.sellerId);
        PlayerEconomy.AddCurrency(sellerWallet, offer.askingPrice, offer.currency);
        
        // Transfer items (simplified - real implementation would handle inventory)
        PlayerEconomy.TransferItems(offer.sellerId, buyerId, offer.items);
        
        // Update offer status
        offer.buyerId = buyerId;
        offer.status = TransactionStatus.Completed;
        activeOffers[offerIndex] = offer;
        
        // Update reputations
        PlayerEconomy.UpdateTradeReputation(offer.sellerId, 5);
        PlayerEconomy.UpdateTradeReputation(buyerId, 3);
        
        // Update market data
        for item in offer.items {
            PlayerEconomy.RecordTransaction(item.category, offer.askingPrice / ArraySize(offer.items));
        }
        
        LogChannel(n"COOP_ECONOMY", "Direct transaction completed: " + offer.offerId);
        
        // Broadcast completion
        let transactionData = offer.offerId + "|" + buyerId + "|completed";
        NetworkingSystem.BroadcastMessage("trade_offer_accepted", transactionData);
        
        return true;
    }
    
    private static func ProcessEscrowTransaction(offerIndex: Int32, buyerId: String) -> Bool {
        let offer = activeOffers[offerIndex];
        
        // Move buyer's money to escrow
        let buyerWallet = PlayerEconomy.GetPlayerWallet(buyerId);
        PlayerEconomy.DeductCurrency(buyerWallet, offer.askingPrice, offer.currency);
        buyerWallet.escrowBalance += offer.askingPrice;
        PlayerEconomy.UpdatePlayerWallet(buyerWallet);
        
        // Set offer to pending
        offer.buyerId = buyerId;
        offer.status = TransactionStatus.Pending;
        activeOffers[offerIndex] = offer;
        
        LogChannel(n"COOP_ECONOMY", "Escrow transaction initiated: " + offer.offerId);
        
        // Schedule automatic completion after meeting
        DelaySystem.DelayCallback(PlayerEconomy.CompleteEscrowTransaction, 300.0, offerIndex); // 5 minutes
        
        return true;
    }
    
    private static func CompleteEscrowTransaction(offerIndex: Int32) -> Void {
        if offerIndex >= ArraySize(activeOffers) {
            return; // Offer may have been removed
        }
        
        let offer = activeOffers[offerIndex];
        
        if offer.status != TransactionStatus.Pending {
            return; // Already completed or cancelled
        }
        
        // Release escrow to seller
        let buyerWallet = PlayerEconomy.GetPlayerWallet(offer.buyerId);
        let sellerWallet = PlayerEconomy.GetPlayerWallet(offer.sellerId);
        
        buyerWallet.escrowBalance -= offer.askingPrice;
        PlayerEconomy.AddCurrency(sellerWallet, offer.askingPrice, offer.currency);
        
        PlayerEconomy.UpdatePlayerWallet(buyerWallet);
        PlayerEconomy.UpdatePlayerWallet(sellerWallet);
        
        // Transfer items
        PlayerEconomy.TransferItems(offer.sellerId, offer.buyerId, offer.items);
        
        // Complete transaction
        offer.status = TransactionStatus.Completed;
        activeOffers[offerIndex] = offer;
        
        LogChannel(n"COOP_ECONOMY", "Escrow transaction completed: " + offer.offerId);
    }
    
    public static func CreateAuction(sellerId: String, item: TradableItem, startingBid: Int32, reservePrice: Int32, duration: Float) -> String {
        let auctionId = "auction_" + sellerId + "_" + ToString(GetGameTime());
        
        let auction: AuctionListing;
        auction.auctionId = auctionId;
        auction.sellerId = sellerId;
        auction.item = item;
        auction.startingBid = startingBid;
        auction.currentBid = startingBid;
        auction.highestBidderId = "";
        auction.reservePrice = reservePrice;
        auction.bidIncrement = MaxI(100, startingBid / 20); // 5% of starting bid
        auction.endTime = GetGameTime() + duration;
        auction.isActive = true;
        auction.category = item.category;
        auction.featuredListing = item.quality == "legendary" || item.marketValue >= 50000;
        
        ArrayPush(auctionListings, auction);
        
        LogChannel(n"COOP_ECONOMY", "Created auction: " + auctionId + " for " + item.itemName);
        
        return auctionId;
    }
    
    public static func PlaceBid(auctionId: String, bidderId: String, bidAmount: Int32, isProxy: Bool, maxProxyAmount: Int32) -> Bool {
        let auctionIndex = PlayerEconomy.FindAuctionIndex(auctionId);
        if auctionIndex == -1 {
            LogChannel(n"COOP_ECONOMY", "Auction not found: " + auctionId);
            return false;
        }
        
        let auction = auctionListings[auctionIndex];
        
        if !auction.isActive || GetGameTime() >= auction.endTime {
            LogChannel(n"COOP_ECONOMY", "Auction is not active");
            return false;
        }
        
        if bidAmount <= auction.currentBid {
            LogChannel(n"COOP_ECONOMY", "Bid must be higher than current bid");
            return false;
        }
        
        if bidAmount < (auction.currentBid + auction.bidIncrement) {
            LogChannel(n"COOP_ECONOMY", "Bid does not meet minimum increment");
            return false;
        }
        
        if Equals(auction.sellerId, bidderId) {
            LogChannel(n"COOP_ECONOMY", "Seller cannot bid on own auction");
            return false;
        }
        
        // Check if bidder can afford it
        let bidderWallet = PlayerEconomy.GetPlayerWallet(bidderId);
        if bidderWallet.eddies < bidAmount {
            LogChannel(n"COOP_ECONOMY", "Insufficient funds for bid");
            return false;
        }
        
        // Create bid record
        let bid: AuctionBid;
        bid.bidderId = bidderId;
        bid.bidAmount = bidAmount;
        bid.bidTime = GetGameTime();
        bid.isProxy = isProxy;
        bid.maxProxyAmount = maxProxyAmount;
        
        ArrayPush(auction.bidHistory, bid);
        
        // Update auction
        auction.currentBid = bidAmount;
        auction.highestBidderId = bidderId;
        
        auctionListings[auctionIndex] = auction;
        
        LogChannel(n"COOP_ECONOMY", "Bid placed: " + ToString(bidAmount) + " by " + bidderId + " on " + auctionId);
        
        // Extend auction if bid placed in last 5 minutes
        let timeRemaining = auction.endTime - GetGameTime();
        if timeRemaining < 300.0 {
            auction.endTime += 300.0; // Add 5 minutes
            auctionListings[auctionIndex] = auction;
        }
        
        // Broadcast bid
        let bidData = auctionId + "|" + bidderId + "|" + ToString(bidAmount);
        NetworkingSystem.BroadcastMessage("auction_bid", bidData);
        
        return true;
    }
    
    public static func CreatePlayerShop(ownerId: String, shopName: String, location: Vector3, specialization: array<String>, marketType: MarketType) -> String {
        let shopId = "shop_" + ownerId + "_" + ToString(GetGameTime());
        
        let shop: ShopListing;
        shop.shopId = shopId;
        shop.ownerId = ownerId;
        shop.shopName = shopName;
        shop.location = location;
        shop.marketType = marketType;
        shop.specialization = specialization;
        shop.reputation = 100; // Starting reputation
        shop.openHours = "24/7"; // Always open initially
        shop.isPlayerOwned = true;
        shop.securityLevel = 1; // Basic security
        shop.dailyRevenue = 0;
        shop.monthlyRevenue = 0;
        
        ArrayPush(playerShops, shop);
        
        LogChannel(n"COOP_ECONOMY", "Created player shop: " + shopName + " by " + ownerId);
        
        return shopId;
    }
    
    // Market analysis and pricing
    public static func GetMarketPrice(itemId: String) -> Int32 {
        for market in marketData {
            if Equals(market.itemId, itemId) {
                // Apply economic events
                let adjustedPrice = Cast<Float>(market.averagePrice);
                
                for event in economicEvents {
                    if PlayerEconomy.IsEventActive(event) && PlayerEconomy.IsItemAffected(event, itemId) {
                        adjustedPrice *= event.priceMultiplier;
                    }
                }
                
                return Cast<Int32>(adjustedPrice);
            }
        }
        
        return 1000; // Default price
    }
    
    public static func GetPriceHistory(itemId: String, days: Int32) -> array<PricePoint> {
        for market in marketData {
            if Equals(market.itemId, itemId) {
                let recentHistory: array<PricePoint>;
                let cutoffTime = GetGameTime() - (Cast<Float>(days) * 86400.0);
                
                for pricePoint in market.priceHistory {
                    if pricePoint.timestamp >= cutoffTime {
                        ArrayPush(recentHistory, pricePoint);
                    }
                }
                
                return recentHistory;
            }
        }
        
        let emptyHistory: array<PricePoint>;
        return emptyHistory;
    }
    
    // Update loop
    private static func StartUpdateLoop() -> Void {
        PlayerEconomy.UpdateEconomy();
        DelaySystem.DelayCallback(PlayerEconomy.UpdateLoop, updateInterval);
    }
    
    private static func UpdateLoop() -> Void {
        if !isInitialized {
            return;
        }
        
        let currentTime = GetGameTime();
        if (currentTime - lastUpdateTime) >= updateInterval {
            PlayerEconomy.UpdateEconomy();
            lastUpdateTime = currentTime;
        }
        
        DelaySystem.DelayCallback(PlayerEconomy.UpdateLoop, updateInterval);
    }
    
    private static func UpdateEconomy() -> Void {
        // Update market prices
        PlayerEconomy.UpdateMarketPrices();
        
        // Process expired offers
        PlayerEconomy.ProcessExpiredOffers();
        
        // Process auction endings
        PlayerEconomy.ProcessAuctionEndings();
        
        // Update economic events
        PlayerEconomy.UpdateEconomicEvents();
        
        // Update shop revenues
        PlayerEconomy.UpdateShopRevenues();
    }
    
    private static func UpdateMarketPrices() -> Void {
        for i in Range(ArraySize(marketData)) {
            let market = marketData[i];
            
            // Natural price fluctuation
            let fluctuation = RandRangeF(-0.05, 0.05); // 5% max change
            let newPrice = Cast<Int32>(Cast<Float>(market.averagePrice) * (1.0 + fluctuation));
            
            // Record price change
            let pricePoint: PricePoint;
            pricePoint.price = newPrice;
            pricePoint.timestamp = GetGameTime();
            pricePoint.volume = RandRange(1, 10);
            
            ArrayPush(market.priceHistory, pricePoint);
            
            // Keep only last 30 price points
            if ArraySize(market.priceHistory) > 30 {
                ArrayErase(market.priceHistory, 0);
            }
            
            market.averagePrice = newPrice;
            marketData[i] = market;
        }
    }
    
    private static func ProcessExpiredOffers() -> Void {
        for i in Range(ArraySize(activeOffers)) {
            let offer = activeOffers[i];
            
            if offer.status == TransactionStatus.Active && 
               GetGameTime() >= (offer.createdTime + offer.duration) {
                offer.status = TransactionStatus.Expired;
                activeOffers[i] = offer;
                
                LogChannel(n"COOP_ECONOMY", "Trade offer expired: " + offer.offerId);
            }
        }
    }
    
    private static func ProcessAuctionEndings() -> Void {
        for i in Range(ArraySize(auctionListings)) {
            let auction = auctionListings[i];
            
            if auction.isActive && GetGameTime() >= auction.endTime {
                PlayerEconomy.EndAuction(i);
            }
        }
    }
    
    private static func EndAuction(auctionIndex: Int32) -> Void {
        let auction = auctionListings[auctionIndex];
        auction.isActive = false;
        
        if !Equals(auction.highestBidderId, "") && auction.currentBid >= auction.reservePrice {
            // Auction successful
            PlayerEconomy.CompleteAuctionSale(auction);
            LogChannel(n"COOP_ECONOMY", "Auction completed: " + auction.auctionId + " sold for " + ToString(auction.currentBid));
        } else {
            // Auction failed
            LogChannel(n"COOP_ECONOMY", "Auction failed: " + auction.auctionId + " - reserve not met");
        }
        
        auctionListings[auctionIndex] = auction;
    }
    
    // Utility functions
    private static func GetPlayerWallet(playerId: String) -> PlayerWallet {
        for wallet in playerWallets {
            if Equals(wallet.playerId, playerId) {
                return wallet;
            }
        }
        
        let emptyWallet: PlayerWallet;
        return emptyWallet;
    }
    
    private static func UpdatePlayerWallet(updatedWallet: PlayerWallet) -> Void {
        for i in Range(ArraySize(playerWallets)) {
            if Equals(playerWallets[i].playerId, updatedWallet.playerId) {
                playerWallets[i] = updatedWallet;
                break;
            }
        }
    }
    
    private static func CanAfford(wallet: PlayerWallet, amount: Int32, currency: String) -> Bool {
        switch currency {
            case "eddies":
                return wallet.eddies >= amount;
            case "favor_tokens":
                return wallet.favorTokens >= amount;
            case "data_shards":
                return wallet.dataShards >= amount;
            case "crypto":
                return wallet.cryptoCurrency >= amount;
            default:
                return false;
        }
    }
    
    private static func FindOfferIndex(offerId: String) -> Int32 {
        for i in Range(ArraySize(activeOffers)) {
            if Equals(activeOffers[i].offerId, offerId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func FindAuctionIndex(auctionId: String) -> Int32 {
        for i in Range(ArraySize(auctionListings)) {
            if Equals(auctionListings[i].auctionId, auctionId) {
                return i;
            }
        }
        return -1;
    }
    
    // Network event handlers
    private static cb func OnTradeOfferCreated(data: String) -> Void {
        LogChannel(n"COOP_ECONOMY", "Received trade offer: " + data);
        let offer = PlayerEconomy.DeserializeTradeOffer(data);
        ArrayPush(activeOffers, offer);
    }
    
    private static cb func OnTradeOfferAccepted(data: String) -> Void {
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 2 {
            let offerId = parts[0];
            let buyerId = parts[1];
            PlayerEconomy.AcceptTradeOffer(offerId, buyerId);
        }
    }
    
    private static cb func OnAuctionBid(data: String) -> Void {
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 3 {
            let auctionId = parts[0];
            let bidderId = parts[1];
            let bidAmount = StringToInt(parts[2]);
            PlayerEconomy.PlaceBid(auctionId, bidderId, bidAmount, false, 0);
        }
    }
    
    private static cb func OnMarketTransaction(data: String) -> Void {
        LogChannel(n"COOP_ECONOMY", "Market transaction: " + data);
    }
    
    private static cb func OnShopUpdate(data: String) -> Void {
        LogChannel(n"COOP_ECONOMY", "Shop update: " + data);
    }
    
    // Serialization functions
    private static func SerializeTradeOffer(offer: TradeOffer) -> String {
        let data = offer.offerId + "|" + offer.sellerId + "|" + offer.title + "|" + ToString(offer.askingPrice);
        data += "|" + offer.currency + "|" + ToString(Cast<Int32>(offer.offerType)) + "|" + ToString(Cast<Int32>(offer.marketType));
        return data;
    }
    
    private static func DeserializeTradeOffer(data: String) -> TradeOffer {
        let offer: TradeOffer;
        let parts = StrSplit(data, "|");
        
        if ArraySize(parts) >= 7 {
            offer.offerId = parts[0];
            offer.sellerId = parts[1];
            offer.title = parts[2];
            offer.askingPrice = StringToInt(parts[3]);
            offer.currency = parts[4];
            offer.offerType = IntToEnum(StringToInt(parts[5]), TradeOfferType.Item);
            offer.marketType = IntToEnum(StringToInt(parts[6]), MarketType.Legal);
        }
        
        return offer;
    }
    
    // Public API
    public static func GetActiveOffers(marketType: MarketType) -> array<TradeOffer> {
        let filteredOffers: array<TradeOffer>;
        for offer in activeOffers {
            if offer.marketType == marketType && offer.status == TransactionStatus.Active {
                ArrayPush(filteredOffers, offer);
            }
        }
        return filteredOffers;
    }
    
    public static func GetActiveAuctions() -> array<AuctionListing> {
        let activeAuctions: array<AuctionListing>;
        for auction in auctionListings {
            if auction.isActive {
                ArrayPush(activeAuctions, auction);
            }
        }
        return activeAuctions;
    }
    
    public static func GetPlayerShops() -> array<ShopListing> {
        return playerShops;
    }
    
    public static func GetMarketData(itemId: String) -> MarketData {
        for market in marketData {
            if Equals(market.itemId, itemId) {
                return market;
            }
        }
        
        let emptyMarket: MarketData;
        return emptyMarket;
    }
    
    public static func GetPlayerEconomyStats(playerId: String) -> PlayerEconomyStats {
        let stats: PlayerEconomyStats;
        stats.playerId = playerId;
        
        let wallet = PlayerEconomy.GetPlayerWallet(playerId);
        stats.totalWealth = wallet.eddies + (wallet.favorTokens * 100) + (wallet.dataShards * 50);
        stats.tradeReputation = wallet.tradeReputation;
        
        // Calculate trade activity
        for offer in activeOffers {
            if Equals(offer.sellerId, playerId) {
                stats.activeOffers += 1;
            }
            if Equals(offer.buyerId, playerId) && offer.status == TransactionStatus.Completed {
                stats.completedPurchases += 1;
            }
        }
        
        return stats;
    }
}

// Supporting structures
public struct PlayerEconomyStats {
    public var playerId: String;
    public var totalWealth: Int32;
    public var tradeReputation: Int32;
    public var activeOffers: Int32;
    public var completedSales: Int32;
    public var completedPurchases: Int32;
    public var averageSalePrice: Int32;
    public var totalProfit: Int32;
}