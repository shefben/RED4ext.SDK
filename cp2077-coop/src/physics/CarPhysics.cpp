#include "CarPhysics.hpp"
#include "../core/GameClock.hpp"
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
    snap.pos += snap.vel * dt;
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
    snap.pos += snap.vel * dt;
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
} // namespace CoopNet
