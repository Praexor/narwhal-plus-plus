#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace narwhal::utils {

template<typename T>
class Channel {
public:
    void send(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(std::move(value));
        }
        cv.notify_one();
    }

    std::optional<T> receive() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return !queue.empty() || closed; });
        
        if (queue.empty() && closed) {
            return std::nullopt;
        }

        T value = std::move(queue.front());
        queue.pop();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            closed = true;
        }
        cv.notify_all();
    }

private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable cv;
    bool closed = false;
};

} // namespace narwhal::utils
