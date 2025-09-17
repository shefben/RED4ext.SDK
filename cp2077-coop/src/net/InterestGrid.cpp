#include "InterestGrid.hpp"

namespace CoopNet {

void InterestGrid::Insert(uint32_t id, const RED4ext::Vector3& pos)
{
    std::lock_guard lock(m_mutex);
    m_posMap[id] = pos;
    m_grid.Insert(id, pos);
}

void InterestGrid::Move(uint32_t id, const RED4ext::Vector3& pos)
{
    std::lock_guard lock(m_mutex);
    auto it = m_posMap.find(id);
    if (it != m_posMap.end()) {
        m_grid.Move(id, it->second, pos);
        it->second = pos;
    } else {
        m_posMap[id] = pos;
        m_grid.Insert(id, pos);
    }
}

void InterestGrid::Remove(uint32_t id)
{
    std::lock_guard lock(m_mutex);
    auto it = m_posMap.find(id);
    if (it != m_posMap.end()) {
        m_grid.Remove(id, it->second);
        m_posMap.erase(it);
    }
}

void InterestGrid::Query(const RED4ext::Vector3& center, float radius, std::vector<uint32_t>& out) const
{
    std::lock_guard lock(m_mutex);
    m_grid.QueryCircle(center, radius, out);
}

size_t InterestGrid::GetSize() const
{
    std::lock_guard lock(m_mutex);
    return m_posMap.size();
}

InterestGrid g_interestGrid;

} // namespace CoopNet
