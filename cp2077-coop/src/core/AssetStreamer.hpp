#pragma once
#include "ThreadSafeQueue.hpp"
#include <cstdint>
#include <vector>
#include <atomic>
#include <thread>

namespace CoopNet
{
class AssetStreamer
{
public:
    struct Task
    {
        uint16_t pluginId;
        std::vector<uint8_t> data;
    };
    struct Result
    {
        uint16_t pluginId;
        bool success;
    };

    AssetStreamer();
    ~AssetStreamer();

    void Start();
    void Stop();
    void Submit(Task&& t);
    bool Poll(Result& out);
    size_t GetPending() const;

private:
    void Worker();
    bool Process(const Task& t);

    ThreadSafeQueue<Task> m_tasks;
    ThreadSafeQueue<Result> m_results;
    std::jthread m_thread;
    std::atomic<bool> m_running{false};
};

AssetStreamer& GetAssetStreamer();
} // namespace CoopNet
