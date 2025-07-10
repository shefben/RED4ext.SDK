#include "CarPhysics.hpp"

namespace CoopNet
{
// Run physics at a fixed step regardless of frame delta.
// `accumMs` carries leftover time between frames.
void StepVehicle(TransformSnap& state, float& accumMs, float dtMs, bool authoritative)
{
    accumMs += dtMs;
    while (accumMs >= kVehicleStepMs)
    {
        if (authoritative)
            ServerSimulate(state, kVehicleStepMs);
        else
            ClientPredict(state, kVehicleStepMs);
        accumMs -= kVehicleStepMs;
    }
}
} // namespace CoopNet
