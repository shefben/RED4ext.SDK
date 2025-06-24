#pragma once

// Networking layer for cp2077-coop.
// Provides thin wrappers around ENet.
#include <cstdint>
void Net_Init();
void Net_Shutdown();
void Net_Poll(uint32_t maxMs);
bool Net_IsAuthoritative();
