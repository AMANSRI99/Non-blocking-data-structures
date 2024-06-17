#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "nbq.cpp"  // Ensure this includes the correct LockFreeQueue implementation

// Function for producer threads
void producer(LockFreeQueue<int>& queue, int num_elements, std::atomic<int>& enqueued_count, std::atomic<int>& done_producing) {
    for (int i = 0; i < num_elements; ++i) {
        queue.enqueue(i);
        ++enqueued_count;
    }
    ++done_producing;
}

// Function for consumer threads
void consumer(LockFreeQueue<int>& queue, std::atomic<int>& dequeued_count, std::atomic<int>& done_producing, int total_producers) {
    int value;
    while (true) {
        if (queue.dequeue(value)) {
            ++dequeued_count;
        } else {
            // Check if all producers are done and queue is empty
            if (done_producing.load() == total_producers) {
                break;
            }
            std::this_thread::yield();  // Yield the thread to reduce CPU usage
        }
    }

    // Final check to ensure the queue is empty after producers are done
    while (queue.dequeue(value)) {
        ++dequeued_count;
    }
}

int main() {
    const int num_elements = 1000000;   // Total number of elements to enqueue/dequeue
    const int num_producers = 4;        // Number of producer threads
    const int num_consumers = 4;        // Number of consumer threads

    LockFreeQueue<int> queue;
    std::atomic<int> enqueued_count(0);
    std::atomic<int> dequeued_count(0);
    std::atomic<int> done_producing(0);

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Start producer threads
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back(producer, std::ref(queue), num_elements / num_producers, std::ref(enqueued_count), std::ref(done_producing));
    }

    // Start consumer threads
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back(consumer, std::ref(queue), std::ref(dequeued_count), std::ref(done_producing), num_producers);
    }

    // Join producer threads
    for (auto& t : producers) {
        t.join();
    }

    // Join consumer threads
    for (auto& t : consumers) {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    // Ensure the output is flushed correctly
    std::cout << "Total Enqueued: " << enqueued_count.load() << std::endl;
    std::cout << "Total Dequeued: " << dequeued_count.load() << std::endl;
    std::cout << "Time taken: " << elapsed.count() << " seconds" << std::endl;

    return 0;
}
