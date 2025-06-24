#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <type_traits>
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>

namespace CoopNet
{

// Unique identifier for a simulation snapshot.
using SnapshotId = uint32_t;

// Bitset describing which fields changed relative to the baseline.
// 128 bits allow for future expansion of replicated properties.
// Bits 0..5 are reserved for TransformSnap
//   0 - position
//   1 - velocity
//   2 - rotation
//   3 - health
//   4 - armor
//   5 - ownerId
//   6 - ackSeq
struct SnapshotFieldFlags
{
    uint32_t bits[4]; // 4 * 32 = 128 flags
};
static_assert(sizeof(SnapshotFieldFlags) == 16, "flags must be 128 bits");

constexpr uint32_t kMaxSnapshotFields = 128;

// Basic header stored before each snapshot payload.
// `id` is the absolute snapshot index.
// `baseId` points to the previous snapshot used as a baseline when delta-compressing.
struct SnapshotHeader
{
    SnapshotId id;
    SnapshotId baseId;
};
static_assert(sizeof(SnapshotHeader) == 8, "header must be 8 bytes");

// Minimal transform data replicated for remote avatars.
struct TransformSnap
{
    Vector3 pos;      // field flag index 0
    Vector3 vel;      // field flag index 1
    Quaternion rot;   // field flag index 2
    uint16_t health;  // field flag index 3
    uint16_t armor;   // field flag index 4
    uint32_t ownerId; // field flag index 5
    uint16_t seq;     // field flag index 6
};
static_assert(std::is_trivially_copyable_v<TransformSnap>, "TransformSnap must be trivial");

// NPC state replicated from the server. Position, rotation, state, and health
// use delta bits while templateId and appearanceSeed always send full values.
// health == 0 implies the NPC should despawn.
struct NpcSnap
{
    uint32_t npcId;         // always included
    uint16_t templateId;    // full snap only
    Vector3  pos;           // delta bit 0
    Quaternion rot;         // delta bit 1
    uint8_t  state;         // delta bit 2 (Idle, Wander, Combat, ...)
    uint16_t health;        // delta bit 3
    uint8_t  appearanceSeed; // full snap only
};
static_assert(sizeof(NpcSnap) % 4 == 0, "NpcSnap must align to 4 bytes");
static_assert(std::is_trivially_copyable_v<NpcSnap>, "NpcSnap must be trivial");

// Writes snapshot data into a buffer with dirty-bit tracking.
// Begin() resets internal state with the target header.
// Write<T>() serializes a field and marks the bit in SnapshotFieldFlags.
// End() finalizes the buffer as a delta against `baseId` so multiple
// snapshots may form a chain of baselines for compression.
class SnapshotWriter
{
public:
    // Reset writer state for a new snapshot
    void Begin(const SnapshotHeader& header)
    {
        m_header = header;
        std::memset(&m_flags, 0, sizeof(m_flags));
        m_payload.clear();
    }

    // Write a trivially-copyable value and mark its dirty flag
    template<typename T>
    void Write(uint32_t fieldIndex, const T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>, "snapshot values must be POD");

        if (fieldIndex >= kMaxSnapshotFields)
            return; // ignore out-of-range field bits

        const uint32_t word = fieldIndex / 32;
        const uint32_t bit  = fieldIndex % 32;
        m_flags.bits[word] |= (1u << bit);

        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&value);
        m_payload.insert(m_payload.end(), ptr, ptr + sizeof(T));
    }

    // Finalize the snapshot into the output buffer; returns bytes written or 0 if insufficient space
    size_t End(uint8_t* outBuf, size_t max) const
    {
        const size_t total = sizeof(SnapshotHeader) + sizeof(SnapshotFieldFlags) + m_payload.size();
        if (max < total)
            return 0;

        std::memcpy(outBuf, &m_header, sizeof(SnapshotHeader));
        std::memcpy(outBuf + sizeof(SnapshotHeader), &m_flags, sizeof(SnapshotFieldFlags));
        if (!m_payload.empty())
        {
            std::memcpy(outBuf + sizeof(SnapshotHeader) + sizeof(SnapshotFieldFlags), m_payload.data(), m_payload.size());
        }
        return total;
    }

private:
    SnapshotHeader m_header{};
    SnapshotFieldFlags m_flags{};
    std::vector<uint8_t> m_payload;
};

// Reads snapshot data from a buffer and resolves deltas against a baseline chain.
// Attach() selects the memory buffer containing the snapshot payload.
// Read<T>() retrieves a value from the attached buffer. When a field is not
// flagged as dirty the reader walks back through `baseId` snapshots until a
// value is found, enabling compact delta compression.
class SnapshotReader
{
public:
    void Attach(const uint8_t* buffer, size_t size)
    {
        m_buffer = buffer;
        m_size = size;
        if (size >= sizeof(SnapshotHeader) + sizeof(SnapshotFieldFlags))
        {
            std::memcpy(&m_header, buffer, sizeof(SnapshotHeader));
            std::memcpy(&m_flags, buffer + sizeof(SnapshotHeader), sizeof(SnapshotFieldFlags));
            m_cursor = buffer + sizeof(SnapshotHeader) + sizeof(SnapshotFieldFlags);
        }
    }

    bool Has(uint32_t fieldIndex) const
    {
        const uint32_t word = fieldIndex / 32;
        const uint32_t bit  = fieldIndex % 32;
        return (m_flags.bits[word] & (1u << bit)) != 0;
    }

    template<typename T>
    T Read()
    {
        T out{};
        if (m_cursor + sizeof(T) <= m_buffer + m_size)
        {
            std::memcpy(&out, m_cursor, sizeof(T));
            m_cursor += sizeof(T);
        }
        return out;
    }

    SnapshotId GetBaseId() const { return m_header.baseId; }

private:
    const uint8_t* m_buffer = nullptr;
    const uint8_t* m_cursor = nullptr;
    size_t m_size = 0;
    SnapshotHeader m_header{};
    SnapshotFieldFlags m_flags{};
};

} // namespace CoopNet

