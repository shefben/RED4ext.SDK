#pragma once

#include "../net/Snapshot.hpp"
#include <algorithm>

namespace CoopNet
{
constexpr float kVehicleStepMs = 16.f; // 60 Hz
// Server authoritative car physics integration.
void ServerSimulate(TransformSnap& inout, float dtMs);

// Client-side prediction using the same integration step.
void ClientPredict(TransformSnap& inout, float dtMs);

// High-speed collision resolution with latency rewind (VC-1)
void HandleHighSpeedCollision(uint32_t idA, TransformSnap& a, float latencyAms, uint32_t idB, TransformSnap& b,
                              float latencyBms);
} // namespace CoopNet
