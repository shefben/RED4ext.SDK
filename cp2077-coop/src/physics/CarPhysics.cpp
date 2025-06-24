#include "CarPhysics.hpp"

namespace CoopNet
{
// Fixed-step Euler integration placeholder for vehicles.
// When both server and client use the same dtMs the drift in position
// should stay under a few centimeters after long runs. Any mismatch in
// the step size will accumulate error over time.
void ServerSimulate(TransformSnap& snap, float dtMs)
{
    float dt = dtMs / 1000.f;
    snap.pos += snap.vel * dt;
    // TODO(next ticket): integrate rotation and forces
}

void ClientPredict(TransformSnap& snap, float dtMs)
{
    float dt = dtMs / 1000.f;
    snap.pos += snap.vel * dt;
}
} // namespace CoopNet
