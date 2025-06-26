#include "SpatialGrid.hpp"
#include <cmath>
#include <algorithm>

namespace CoopNet
{
namespace
{
inline bool CircleIntersects(const RED4ext::Vector3& c, float r, const RED4ext::Vector3& min, const RED4ext::Vector3& max)
{
    float x = std::clamp(c.X, min.X, max.X);
    float y = std::clamp(c.Y, min.Y, max.Y);
    float dx = c.X - x;
    float dy = c.Y - y;
    return dx * dx + dy * dy <= r * r;
}
}

SpatialGrid::SpatialGrid()
{
    m_root = std::make_unique<QuadNode>();
    m_root->min = {-512.f, -512.f, -100.f};
    m_root->max = {512.f, 512.f, 100.f};
}

void SpatialGrid::Insert(uint32_t id, const RED4ext::Vector3& pos)
{
    InsertRec(m_root.get(), id, pos, 0);
}

void SpatialGrid::Move(uint32_t id, const RED4ext::Vector3& oldPos, const RED4ext::Vector3& newPos)
{
    Remove(id, oldPos);
    Insert(id, newPos);
}

void SpatialGrid::Remove(uint32_t id, const RED4ext::Vector3& pos)
{
    RemoveRec(m_root.get(), id, pos);
}

void SpatialGrid::QueryCircle(const RED4ext::Vector3& center, float radius, std::vector<uint32_t>& outIds) const
{
    outIds.clear();
    QueryRec(m_root.get(), center, radius, outIds);
}

void SpatialGrid::InsertRec(QuadNode* node, uint32_t id, const RED4ext::Vector3& pos, uint32_t depth)
{
    if (depth >= 6 || (node->child[0] == nullptr && node->ids.size() < kNodeCapacity))
    {
        node->ids.push_back(id);
        return;
    }
    if (node->child[0] == nullptr)
        Subdivide(node, depth);
    for (int i = 0; i < 4; ++i)
    {
        auto& c = node->child[i];
        if (pos.X >= c->min.X && pos.X <= c->max.X && pos.Y >= c->min.Y && pos.Y <= c->max.Y)
        {
            InsertRec(c.get(), id, pos, depth + 1);
            return;
        }
    }
    node->ids.push_back(id);
}

bool SpatialGrid::RemoveRec(QuadNode* node, uint32_t id, const RED4ext::Vector3& pos)
{
    auto it = std::find(node->ids.begin(), node->ids.end(), id);
    if (it != node->ids.end())
    {
        node->ids.erase(it);
        return true;
    }
    for (int i = 0; i < 4; ++i)
    {
        auto& c = node->child[i];
        if (c && pos.X >= c->min.X && pos.X <= c->max.X && pos.Y >= c->min.Y && pos.Y <= c->max.Y)
        {
            if (RemoveRec(c.get(), id, pos))
                return true;
        }
    }
    return false;
}

void SpatialGrid::QueryRec(const QuadNode* node, const RED4ext::Vector3& center, float radius, std::vector<uint32_t>& outIds) const
{
    if (!CircleIntersects(center, radius, node->min, node->max))
        return;
    outIds.insert(outIds.end(), node->ids.begin(), node->ids.end());
    for (int i = 0; i < 4; ++i)
        if (node->child[i])
            QueryRec(node->child[i].get(), center, radius, outIds);
}

void SpatialGrid::Subdivide(QuadNode* node, uint32_t depth)
{
    RED4ext::Vector3 half{(node->max.X - node->min.X) * 0.5f, (node->max.Y - node->min.Y) * 0.5f, (node->max.Z - node->min.Z)};
    for (int i = 0; i < 4; ++i)
    {
        node->child[i] = std::make_unique<QuadNode>();
        float offX = (i % 2) ? half.X : 0.f;
        float offY = (i < 2) ? 0.f : half.Y;
        node->child[i]->min = {node->min.X + offX, node->min.Y + offY, node->min.Z};
        node->child[i]->max = {node->child[i]->min.X + half.X, node->child[i]->min.Y + half.Y, node->max.Z};
    }
}

} // namespace CoopNet

} // namespace CoopNet
