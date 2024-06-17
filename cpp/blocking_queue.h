#ifndef BLOCKINGQUEUE_H
#define BLOCKINGQUEUE_H

#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "IQueue.h"

template <typename T>
class BlockingQueue : public IQueue<T> {
public:
    BlockingQueue() : done(false) {}

    void enqueue(T value) override {
        {
            std::lock_guard<std::mutex> lock(mtx);
            q.push(value);
        }
        cv.notify_one();
    }

    bool dequeue(T& value) override {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return done || !q.empty(); });
        if (q.empty()) {
            return false; // Return false if queue is empty and done flag is set
        }
        value = q.front();
        q.pop();
        return true;
    }

    void printQueue() const {
        std::lock_guard<std::mutex> lock(mtx);
        std::queue<T> copy = q; // Make a copy to traverse
        std::cout << "Queue: ";
        while (!copy.empty()) {
            std::cout << copy.front() << " ";
            copy.pop();
        }
        std::cout << std::endl;
    }

    void set_done() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            done = true;
        }
        cv.notify_all();
    }

private:
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::queue<T> q;
    bool done;
};

#endif // BLOCKINGQUEUE_H