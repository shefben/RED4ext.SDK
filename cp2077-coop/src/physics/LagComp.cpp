#include "LagComp.hpp"

// rewind = pos - vel * (clientRTTms / 1000.f)

namespace CoopNet
{
RED4ext::Vector3 RewindPosition(const RED4ext::Vector3& pos,
                                const RED4ext::Vector3& vel,
                                float clientRTTms)
{
    float t = clientRTTms / 1000.f;
    return pos - vel * t;
}
} // namespace CoopNet
