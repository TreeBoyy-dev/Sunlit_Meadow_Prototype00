// ThreadSafeQueue.h
#pragma once
#include <queue>
#include <deque>
#include <mutex>
#include <optional>
#include <functional>

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
        m_queue.emplace_back(std::move(item1), std::move(item2));
    }

    std::optional<std::pair<T1, T2>> try_pop() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_queue.empty())
            return std::nullopt;

        auto item = std::move(m_queue.front());
        m_queue.pop_front();
        return item;
    }

    template<typename Pred>
    bool remove_if(Pred pred) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_queue.begin(); it != m_queue.end(); ++it) {
            if (pred(std::get<0>(*it), std::get<1>(*it))) {
                m_queue.erase(it);
                return true;
            }
        }
        return false;
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
    std::deque<std::pair<T1, T2>> m_queue;
    mutable std::mutex m_mutex;
};