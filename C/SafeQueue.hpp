#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <unordered_set>
#include <atomic>
#include <iostream>

struct QueueArg {
    std::string url;
    std::string html;
};

// A threadsafe-queue.
class SafeQueue {
    private:
        bool unique;
        std::queue<QueueArg> q;
        mutable std::mutex m;
        std::condition_variable c;
        std::unordered_set<std::string> s;

    public:
        SafeQueue(bool unique)
        : q()
        , m()
        , c()
        , s()
        {
            this->unique = unique;
        }

        ~SafeQueue(void) {}

        // Add an element to the queue.
        void enqueue(QueueArg t) {
            std::lock_guard<std::mutex> lock(m);

            // Only add element if unique
            if (!unique || s.find(t.url) == s.end()) {
                if (unique) s.insert(t.url);
                q.push(t);
            }

            c.notify_one();
        }

        // Get the "front"-element.
        // If the queue is empty, wait till an element is avaiable.
        QueueArg dequeue(SafeQueue *otherQueue, const std::atomic<unsigned int> *activeThreads = nullptr) {
            std::unique_lock<std::mutex> lock(m);
            while(q.empty()) {
                if ((!activeThreads && otherQueue->empty()) || (otherQueue->empty() && activeThreads->load() == 0)) {
                    return (QueueArg) { .url = "xxs" };
                }

                // Release lock as long as the wait and reaquire it afterwards.
                c.wait(lock);
            }
            QueueArg t = std::move(q.front());
            q.pop();
            return t;
        }

        void notify_all() {
            c.notify_all();
        }

        bool empty() {
            std::unique_lock<std::mutex> lock(m);
            return q.empty();
        }
};
#endif