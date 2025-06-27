#pragma once
#include "../net/Connection.hpp"

namespace CoopNet
{
void DealerController_HandleBuy(Connection* conn, uint32_t vehicleTpl, uint32_t price);
}
