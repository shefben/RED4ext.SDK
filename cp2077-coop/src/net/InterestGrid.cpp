#include "InterestGrid.hpp"

namespace CoopNet {

void InterestGrid::Insert(uint32_t id, const RED4ext::Vector3& pos)
{
    m_posMap[id] = pos;
    m_grid.Insert(id, pos);
}

void InterestGrid::Move(uint32_t id, const RED4ext::Vector3& pos)
{
    auto it = m_posMap.find(id);
    if (it != m_posMap.end()) {
        m_grid.Move(id, it->second, pos);
        it->second = pos;
    } else {
        Insert(id, pos);
    }
}

void InterestGrid::Remove(uint32_t id)
{
    auto it = m_posMap.find(id);
    if (it != m_posMap.end()) {
        m_grid.Remove(id, it->second);
        m_posMap.erase(it);
    }
}

void InterestGrid::Query(const RED4ext::Vector3& center, float radius, std::vector<uint32_t>& out) const
{
    m_grid.QueryCircle(center, radius, out);
}

size_t InterestGrid::GetSize() const
{
    return m_posMap.size();
}

InterestGrid g_interestGrid;

} // namespace CoopNet
