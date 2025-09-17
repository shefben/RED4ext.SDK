#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

namespace CoopNet
{

template<typename T>
class ThreadSafeQueue
{
private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;

public:
    ThreadSafeQueue() = default;
    
    // Delete copy constructor and assignment operator
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    
    void Push(const T& item)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(item);
        m_condition.notify_one();
    }
    
    void Push(T&& item)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(item));
        m_condition.notify_one();
    }
    
    bool TryPop(T& item)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return false;
        
        item = m_queue.front();
        m_queue.pop();
        return true;
    }
    
    void WaitAndPop(T& item)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty())
        {
            m_condition.wait(lock);
        }
        
        item = m_queue.front();
        m_queue.pop();
    }
    
    // Legacy Pop method for backward compatibility
    bool Pop(T& item)
    {
        return TryPop(item);
    }
    
    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    
    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
    
    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::queue<T> empty;
        m_queue.swap(empty);
    }
};

} // namespace CoopNet