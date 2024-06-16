#include <atomic>
#include <iostream>

template <typename T>
class LockFreeQueue {
public:
    struct Node {
        T value;
        std::atomic<Node*> next;

        Node(T val = T()) : value(val), next(nullptr) {}
    };

    struct Pointer {
        std::atomic<Node*> ptr;
        unsigned count;

        Pointer(Node* node = nullptr, unsigned cnt = 0) : ptr(node), count(cnt) {}
    };

    struct Queue {
        Pointer Head;
        Pointer Tail;
    };

    LockFreeQueue() {
        Node* node = new Node();
        Q.Head.ptr.store(node);
        Q.Tail.ptr.store(node);
    }

    ~LockFreeQueue() {
        while (Node* node = Q.Head.ptr.load()) {
            Q.Head.ptr.store(node->next);
            delete node;
        }
    }

    void enqueue(T value) {
        Node* node = new Node(value);
        while (true) {
            Pointer tail = Q.Tail;
            Node* next = tail.ptr.load()->next.load();
            if (tail.ptr == Q.Tail.ptr) {
                if (next == nullptr) {
                    if (tail.ptr.load()->next.compare_exchange_weak(next, node)) {
                        Q.Tail.compare_exchange_weak(tail, Pointer(node, tail.count + 1));
                        return;
                    }
                } else {
                    Q.Tail.compare_exchange_weak(tail, Pointer(next, tail.count + 1));
                }
            }
        }
    }

    bool dequeue(T& pvalue) {
        while (true) {
            Pointer head = Q.Head;
            Pointer tail = Q.Tail;
            Node* next = head.ptr.load()->next.load();
            if (head.ptr == Q.Head.ptr) {
                if (head.ptr.load() == tail.ptr.load()) {
                    if (next == nullptr) {
                        return false;
                    }
                    Q.Tail.compare_exchange_weak(tail, Pointer(next, tail.count + 1));
                } else {
                    pvalue = next->value;
                    if (Q.Head.compare_exchange_weak(head, Pointer(next, head.count + 1))) {
                        delete head.ptr.load();
                        return true;
                    }
                }
            }
        }
    }

private:
    Queue Q;
};

int main() {
    LockFreeQueue<int> queue;

    // Example usage:
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);

    int value;
    if (queue.dequeue(value)) {
        std::cout << "Dequeued: " << value << std::endl;
    }

    if (queue.dequeue(value)) {
        std::cout << "Dequeued: " << value << std::endl;
    }

    if (queue.dequeue(value)) {
        std::cout << "Dequeued: " << value << std::endl;
    }

    return 0;
}