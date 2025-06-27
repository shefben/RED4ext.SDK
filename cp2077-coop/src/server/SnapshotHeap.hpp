#pragma once
#include <cstddef>

namespace CoopNet
{
void SnapshotStore_Add(size_t bytes);
void SnapshotStore_PurgeOld(float ageSec);
size_t SnapshotStore_GetMemory();
void SnapshotMemCheck();
} // namespace CoopNet
