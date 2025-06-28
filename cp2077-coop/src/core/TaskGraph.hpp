#pragma once
#include "ThreadSafeQueue.hpp"
#include <functional>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>

namespace CoopNet
{
class TaskGraph
{
public:
    TaskGraph();
    ~TaskGraph();

    void Start(size_t workers);
    void Stop();
    void Resize(size_t workers);
    void Submit(const std::function<void()>& task);
    size_t GetWorkerCount() const;

private:
    void WorkerLoop();

    ThreadSafeQueue<std::function<void()>> m_tasks;
    std::vector<std::jthread> m_workers;
    std::atomic<bool> m_running{false};
    mutable std::mutex m_resizeMutex;
};
} // namespace CoopNet
