#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class SafeQueue {
    public:
    void push(const T& message) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(message);
        lock.unlock();
        condition_.notify_one(); // Notify waiting thread
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty()) {
            condition_.wait(lock); // Wait for a message to be available
        }
        T message = queue_.front();
        queue_.pop();
        return message;
    }

    private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

#endif