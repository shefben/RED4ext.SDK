#include "DealerController.hpp"
#include "../net/Net.hpp"
#include "LedgerService.hpp"
#include "SessionState.hpp"
#include "VehicleController.hpp"
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace CoopNet
{
static std::unordered_map<uint32_t, std::unordered_set<uint32_t>> g_owned;

void DealerController_HandleBuy(Connection* conn, uint32_t vehicleTpl, uint32_t price)
{
    uint64_t bal;
    if (!Ledger_Transfer(conn, -static_cast<int64_t>(price), 0, bal))
        return;
    g_owned[conn->peerId].insert(vehicleTpl);
    Net_BroadcastVehicleUnlock(conn->peerId, vehicleTpl);
    std::cout << "DealerBuy tpl=" << vehicleTpl << " by peer=" << conn->peerId << std::endl;
}
} // namespace CoopNet
