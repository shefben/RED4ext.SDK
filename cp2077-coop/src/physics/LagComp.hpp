#pragma once

// Helper for lag compensation on the server.
// Rewinds a target position based on client RTT.

#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>

namespace CoopNet
{
RED4ext::Vector3 RewindPosition(const RED4ext::Vector3& pos,
                                const RED4ext::Vector3& vel,
                                float clientRTTms);
} // namespace CoopNet
