#include <windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstdlib>  // For std::atoi
#include "IQueue.h"
#include "blocking_queue.h"
#include "non_blocking_queue.h"

// Function to set CPU affinity for a thread
void set_thread_affinity(int core_id) {
    DWORD_PTR mask = (1 << core_id);
    HANDLE thread = GetCurrentThread();

    if (!SetThreadAffinityMask(thread, mask)) {
        DWORD error = GetLastError();
        std::cerr << "SetThreadAffinityMask failed with error " << error << std::endl;
    }
}

// Function for producer threads
template <typename QueueType>
void producer(QueueType& queue, int num_elements, std::atomic<int>& enqueued_count, std::atomic<int>& done_producing, int core_id) {
    set_thread_affinity(core_id);  // Set affinity for this thread

    for (int i = 0; i < num_elements; ++i) {
        queue.enqueue(i);
        ++enqueued_count;
    }
    ++done_producing;
}

// Function for consumer threads
template <typename QueueType>
void consumer(QueueType& queue, std::atomic<int>& dequeued_count, std::atomic<int>& done_producing, int total_producers, int core_id) {
    set_thread_affinity(core_id);  // Set affinity for this thread

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

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <num_elements> <num_producers> <num_consumers> <queue_type>" << std::endl;
        std::cerr << "queue_type: 0 for LockFreeQueue, 1 for BlockingQueue" << std::endl;
        return 1;
    }

    const int num_elements = std::atoi(argv[1]);   // Total number of elements to enqueue/dequeue
    const int num_producers = std::atoi(argv[2]);  // Number of producer threads
    const int num_consumers = std::atoi(argv[3]);  // Number of consumer threads
    const int queue_type = std::atoi(argv[4]);     // Queue type: 0 for LockFreeQueue, 1 for BlockingQueue

    if (num_elements <= 0 || num_producers <= 0 || num_consumers <= 0 || (queue_type != 0 && queue_type != 1)) {
        std::cerr << "All parameters must be positive integers and queue_type must be 0 or 1." << std::endl;
        return 1;
    }

    std::unique_ptr<IQueue<int>> queue;
    if (queue_type == 0) {
        queue = std::make_unique<LockFreeQueue<int>>();
    } else {
        queue = std::make_unique<BlockingQueue<int>>();
    }

    std::atomic<int> enqueued_count(0);
    std::atomic<int> dequeued_count(0);
    std::atomic<int> done_producing(0);

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Start producer threads
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back(producer<IQueue<int>>, std::ref(*queue), num_elements / num_producers, std::ref(enqueued_count), std::ref(done_producing), i % std::thread::hardware_concurrency());
    }

    // Start consumer threads
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back(consumer<IQueue<int>>, std::ref(*queue), std::ref(dequeued_count), std::ref(done_producing), num_producers, i % std::thread::hardware_concurrency());
    }

    // Join producer threads
    for (auto& t : producers) {
        t.join();
    }

    // Signal to the BlockingQueue that production is done
    if (queue_type == 1) {
        dynamic_cast<BlockingQueue<int>*>(queue.get())->set_done();
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
