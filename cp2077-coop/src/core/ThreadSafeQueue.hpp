#pragma once
#include <queue>
#include <mutex>

// Generic mutex-protected queue for cross-thread tasks.
// Push from worker threads and pop on the main thread.
// Using a single queue avoids the classic deadlock where the
// game thread waits on the network thread holding the same mutex.

template<typename T>
class ThreadSafeQueue
{
private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;

public:
    void Push(const T& item)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(item);
    }

    bool Pop(T& out)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return false;
        out = m_queue.front();
        m_queue.pop();
        return true;
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
};

