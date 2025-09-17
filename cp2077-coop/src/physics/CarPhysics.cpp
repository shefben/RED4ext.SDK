#include "CarPhysics.hpp"
#include "../core/GameClock.hpp"
#include "../net/Net.hpp"
#include <algorithm>
#include <cmath>

namespace CoopNet
{
// Fixed-step Euler integration for vehicles.
// When both server and client use the same dtMs the drift in position
// should stay under a few centimeters after long runs. Any mismatch in
// the step size will accumulate error over time.
void ServerSimulate(TransformSnap& snap, float dtMs)
{
    float dt = dtMs / 1000.f;
    uint64_t frame = GameClock::GetCurrentTick();
    float noise = std::sinf(static_cast<float>(frame) * 0.1f) * 0.01f;
    // Integrate linear velocity
    snap.pos = snap.pos + snap.vel * dt;
    // Simple friction with deterministic noise
    snap.vel.X = (snap.vel.X + noise) * 0.98f;
    snap.vel.Y = (snap.vel.Y - noise) * 0.98f;
    // Rotate to face velocity direction if moving
    float speed2 = snap.vel.X * snap.vel.X + snap.vel.Y * snap.vel.Y;
    if (speed2 > 0.0001f)
    {
        float yaw = std::atan2f(snap.vel.Y, snap.vel.X);
        float s = std::sinf(yaw * 0.5f);
        float c = std::cosf(yaw * 0.5f);
        snap.rot = {0.f, 0.f, s, c};
    }
}

void ClientPredict(TransformSnap& snap, float dtMs)
{
    float dt = dtMs / 1000.f;
    uint64_t frame = GameClock::GetCurrentTick();
    float noise = std::sinf(static_cast<float>(frame) * 0.1f) * 0.01f;
    snap.pos = snap.pos + snap.vel * dt;
    snap.vel.X = (snap.vel.X + noise) * 0.98f;
    snap.vel.Y = (snap.vel.Y - noise) * 0.98f;
    float speed2 = snap.vel.X * snap.vel.X + snap.vel.Y * snap.vel.Y;
    if (speed2 > 0.0001f)
    {
        float yaw = std::atan2f(snap.vel.Y, snap.vel.X);
        float s = std::sinf(yaw * 0.5f);
        float c = std::cosf(yaw * 0.5f);
        snap.rot = {0.f, 0.f, s, c};
    }
}

static float VecLen(const RED4ext::Vector3& v)
{
    return std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
}

void HandleHighSpeedCollision(uint32_t idA, TransformSnap& a, float latencyAms, uint32_t idB, TransformSnap& b,
                              float latencyBms)
{
    RED4ext::Vector3 diff = a.pos - b.pos;
    float dist2 = diff.X * diff.X + diff.Y * diff.Y + diff.Z * diff.Z;
    if (dist2 > 4.f)
        return;

    RED4ext::Vector3 rel = a.vel - b.vel;
    float impactSpeed = VecLen(rel) * 3.6f; // m/s to km/h
    if (impactSpeed < 200.f)
        return;

    float rewindMs = std::min({latencyAms, latencyBms, 50.f});
    float dt = rewindMs / 1000.f;
    RED4ext::Vector3 posA = a.pos - a.vel * dt;
    RED4ext::Vector3 posB = b.pos - b.vel * dt;
    diff = posA - posB;
    if (diff.X * diff.X + diff.Y * diff.Y + diff.Z * diff.Z > 4.f)
        return;

    RED4ext::Vector3 delta = (b.vel - a.vel) * 0.5f;
    a.vel = a.vel + delta;
    b.vel = b.vel - delta;
    Net_BroadcastVehicleHitHighSpeed(idA, idB, delta);
}
} // namespace CoopNet
