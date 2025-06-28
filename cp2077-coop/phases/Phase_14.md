# Phase 14

```yaml
###############################################################################
# ███  PHASE QS — QUEST & WORLD-STATE SYNCHRONIZATION  (QS-1 … QS-3)        █
###############################################################################

- ticket_id: "QS-1"
  summary: "Enable QuestStage broadcast wire path"
  context_files:
    - path: src/runtime/QuestSync.reds
      excerpt: |
        func SendQuestStageMsg(questName: CName) {      // TODO: send
          LogChannel(n"quest", s"Stage advanced: \(questName)");
          // network send missing
        }
  spec: >
    Finish the SendQuestStageMsg implementation:
      • Append `QuestStage` to `EMsg` enum in `src/net/Packets.hpp`.
      • Create `struct QuestStagePkt { Uint32 nameHash; Uint16 stage; };`
        (hash = FNV1a_32 of questName).
      • In SendQuestStageMsg(), build packet and push through
        Net_SendReliableToAll().
      • In Connection.HandlePacket(), route QuestStage to
        QuestSync.ApplyQuestStage().
  hints:
    - Use helper `FNV1a32(CName)` already defined in utils/hash.reds.

- ticket_id: "QS-2"
  summary: "ApplyQuestStage & divergence guard"
  context_files:
    - path: src/runtime/QuestSync.reds
      excerpt: |
        func ApplyQuestStage(questName: CName) {          // TODO
          // stub
        }
  spec: >
    Complete ApplyQuestStage and add anti-divergence check:
      • Use `QuestSystem.GetInstance(GetGame())` to obtain quest pointer.
      • If local stage < received stage:
          QuestSystem.SetStage(questName, receivedStage).
      • If local stage > receivedStage by >1:
          Log error "[QuestSync] Divergence detected" and request resync
          by sending `QuestResyncRequest`.
      • Add handler for `QuestResyncRequest`; server responds with full
        QuestStage map (existing packet `QuestFullSync`).
  hints:
    - Stage type is `Uint16`; treat 0 = not started.

- ticket_id: "QS-3"
  summary: "Weather & time-of-day broadcast"
  context_files:
    - path: src/runtime/WeatherSync.reds
      excerpt: |
        func BroadcastWorldState() {      // currently logs only
          LogChannel(n"weather", "Sun \(sunAngle)  Weather \(weatherId)");
        }
  spec: >
    Finish world-state transmission:
      • Add `EMsg::WorldState` to Packets (if absent).
      • Packet payload:
          Uint16 sunAngleDeg;   // 0–360 mapped to 0–359
          Uint8  weatherId;     // existing enum
      • Server calls Net_SendUnreliableToAll() every 30 s or when change
        exceeds 5° or weatherId differs.
      • On client, WeatherSync.ApplyWorldState() sets
        TimeSystem.SetSunRotation() and WeatherSystem.SetWeather().
  hints:
    - Round sunAngle to nearest degree to keep packet 3 bytes.



###############################################################################
# ███  PHASE EC — ECONOMY & VENDOR DETAILS  (EC-1 … EC-3)                   █
###############################################################################

- ticket_id: "EC-1"
  summary: "Wallet UI live update after purchase"
  context_files:
    - path: src/runtime/Inventory/Vendor.reds
      excerpt: |
        // PurchaseResultPacket handler
        LogChannel(n"vendor", "Purchase ok");   // TODO wallet UI
  spec: >
    Show credits decrement on screen:
      • Create `src/gui/WalletHud.reds`:
          class WalletHud extends inkHUDLayer {
            var eddies : Int64
            func Update(newBal: Int64);
          }
      • On PurchaseResult handler, call WalletHud.Update().
      • Add simple fade-in “−123 €$” animation on change >0.
  hints:
    - Use `inkText` with style "Medium 32px Bold", colour #FFD93B.

- ticket_id: "EC-2"
  summary: "Vendor dynamic pricing (street-cred & perks)"
  context_files:
    - path: src/runtime/Inventory/Vendor.reds
      excerpt: |
        func CalculatePrice(basePrice: Int32) -> Int32 {  // returns base
          return basePrice;  // TODO perks
        }
  spec: >
    Implement price modifiers:
      • Input vars:
          streetCred = PlayerProgression.GetStreetCredLevel();
          hasPerkCheap = PerkSystem.HasPerk(n"Wholesale");
      • Rules:
          streetCred  1‥50 → price *= (100 - cred)/100  (max −50 %)
          Wholesale perk   → price *= 0.90
      • Clamp to minimum 1.
      • Add unit-test JSON `tests/static/price_cases.json` with five cases.
  hints:
    - Use Int32 math; multiply then /100 to avoid floats.

- ticket_id: "EC-3"
  summary: "Sync vendor stock & cooldown"
  context_files:
    - path: src/runtime/Inventory/Vendor.reds
      excerpt: |
        // TODO restock logic
  spec: >
    Ensure all clients see same vendor inventory:
      • Vendor server instance tracks `stock : Dict<Uint32,Uint16>`.
      • On purchase, decrement stock and broadcast `VendorStockUpdate`
        packet {itemTpl, newQty}.
      • Clients update local UI quantities; if qty==0 disable button.
      • Add hourly restock timer server-side; sends full stock map.
  hints:
    - Use existing `TimerSystem.AddTimer()` API.

###############################################################################
#  💡  RUN ORDER
#  1. QS-1 → QS-2 → QS-3  (quest first, world state next)
#  2. EC-1 → EC-2 → EC-3  (wallet, pricing, stock)
###############################################################################
```