#include "TaskGraph.hpp"
#include <chrono>

namespace CoopNet
{
TaskGraph::TaskGraph() = default;
TaskGraph::~TaskGraph()
{
    Stop();
}

void TaskGraph::WorkerLoop()
{
    while (m_running)
    {
        std::function<void()> task;
        if (m_tasks.Pop(task))
        {
            task();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void TaskGraph::Start(size_t workers)
{
    std::lock_guard<std::mutex> lock(m_resizeMutex);
    if (m_running)
        return;
    m_running = true;
    for (size_t i = 0; i < workers; ++i)
    {
        m_workers.emplace_back([this] { WorkerLoop(); });
    }
}

void TaskGraph::Stop()
{
    std::lock_guard<std::mutex> lock(m_resizeMutex);
    if (!m_running)
        return;
    m_running = false;
    for (auto& t : m_workers)
    {
        if (t.joinable())
            t.join();
    }
    m_workers.clear();
}

void TaskGraph::Resize(size_t workers)
{
    std::lock_guard<std::mutex> lock(m_resizeMutex);
    Stop();
    Start(workers);
}

void TaskGraph::Submit(const std::function<void()>& task)
{
    m_tasks.Push(task);
}

size_t TaskGraph::GetWorkerCount() const
{
    return m_workers.size();
}

} // namespace CoopNet
