#pragma once

#include "../net/Snapshot.hpp"

namespace CoopNet
{
constexpr float kVehicleStepMs = 16.f; // 60 Hz
// Server authoritative car physics integration.
void ServerSimulate(TransformSnap& inout, float dtMs);

// Client-side prediction using the same integration step.
void ClientPredict(TransformSnap& inout, float dtMs);
}
