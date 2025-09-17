#pragma once
#include <RED4ext/Scripting/Natives/Vector3.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace CoopNet
{
struct QuadNode
{
    RED4ext::Vector3 min{};
    RED4ext::Vector3 max{};
    std::vector<uint32_t> ids;
    std::unique_ptr<QuadNode> child[4];
};

class SpatialGrid
{
public:
    static constexpr uint32_t kNodeCapacity = 32;
    SpatialGrid();
    void Insert(uint32_t id, const RED4ext::Vector3& pos);
    void Move(uint32_t id, const RED4ext::Vector3& oldPos, const RED4ext::Vector3& newPos);
    void Remove(uint32_t id, const RED4ext::Vector3& pos);
    void QueryCircle(const RED4ext::Vector3& center, float radius, std::vector<uint32_t>& outIds) const;
    template<typename F>
    void DepthFirst(F&& fn) const
    {
        VisitRec(m_root.get(), 0, fn);
    }

private:
    void InsertRec(QuadNode* node, uint32_t id, const RED4ext::Vector3& pos, uint32_t depth);
    bool RemoveRec(QuadNode* node, uint32_t id, const RED4ext::Vector3& pos);
    void QueryRec(const QuadNode* node, const RED4ext::Vector3& center, float radius, std::vector<uint32_t>& outIds) const;
    void Subdivide(QuadNode* node, uint32_t depth);
    template<typename F>
    void VisitRec(const QuadNode* node, uint32_t depth, F& fn) const
    {
        if (!node)
            return;
        fn(*node, depth);
        for (int i = 0; i < 4; ++i)
            VisitRec(node->child[i].get(), depth + 1, fn);
    }

    std::unique_ptr<QuadNode> m_root;
};
} // namespace CoopNet
