#pragma once
#include "../net/Packets.hpp"

namespace CoopNet
{
class Connection;
void VendorController_Tick(float dt, uint64_t worldClock);
void VendorController_HandlePurchase(CoopNet::Connection* conn, uint32_t vendorId, uint32_t itemId, uint64_t nonce);
} // namespace CoopNet
