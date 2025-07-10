#pragma once
#include "../core/SpatialGrid.hpp"
#include <unordered_map>
#include <vector>

namespace CoopNet {
class InterestGrid {
public:
    void Insert(uint32_t id, const RED4ext::Vector3& pos);
    void Move(uint32_t id, const RED4ext::Vector3& pos);
    void Remove(uint32_t id);
    void Query(const RED4ext::Vector3& center, float radius, std::vector<uint32_t>& out) const;
    size_t GetSize() const;

private:
    SpatialGrid m_grid;
    std::unordered_map<uint32_t, RED4ext::Vector3> m_posMap;
};

extern InterestGrid g_interestGrid;
} // namespace CoopNet
