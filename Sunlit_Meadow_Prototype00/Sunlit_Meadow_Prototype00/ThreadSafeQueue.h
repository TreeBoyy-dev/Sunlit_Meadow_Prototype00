// ThreadSafeQueue.h
#pragma once
#include <queue>
#include <mutex>
#include <optional>

template<typename T>
class ThreadSafeQueue {
public:
    void push(T item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(item));
    }

    // Returns std::nullopt if queue is empty (non-blocking)
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_queue.empty())
            return std::nullopt;

        T item = std::move(m_queue.front());
        m_queue.pop();
        return item;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
};

template<typename T1, typename T2>
class ThreadSafeQueue_2T {
public:
    void push(T1 item1, T2 item2) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.emplace(std::move(item1), std::move(item2));
    }

    std::optional<std::pair<T1, T2>> try_pop() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_queue.empty())
            return std::nullopt;

        auto item = std::move(m_queue.front());
        m_queue.pop();
        return item;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    std::queue<std::pair<T1, T2>> m_queue; 
    mutable std::mutex m_mutex;
};