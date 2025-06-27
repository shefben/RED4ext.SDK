#pragma once
#include "../net/Connection.hpp"

namespace CoopNet
{
struct AptInfo;
void ApartmentController_Load();
void ApartmentController_HandlePurchase(Connection* conn, uint32_t aptId);
const AptInfo* ApartmentController_GetInfo(uint32_t aptId);
bool ApartmentController_IsOwned(uint32_t peerId, uint32_t aptId);
void ApartmentController_HandleEnter(Connection* conn, uint32_t aptId, uint32_t ownerPhaseId);
void ApartmentController_HandlePermChange(Connection* conn, uint32_t aptId, uint32_t targetPeerId, bool allow);
} // namespace CoopNet
