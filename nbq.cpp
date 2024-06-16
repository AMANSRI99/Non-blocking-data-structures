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
        std::atomic<unsigned> count;

        Pointer(Node* node = nullptr, unsigned cnt = 0) : ptr(node), count(cnt) {}
    };

    struct Queue {
        Pointer Head;
        Pointer Tail;
    };

    LockFreeQueue() {
        Node* node = new Node();
        Q.Head.ptr.store(node);
        Q.Head.count.store(0);
        Q.Tail.ptr.store(node);
        Q.Tail.count.store(0);
    }

    ~LockFreeQueue() {
        while (Node* node = Q.Head.ptr.load()) {
            Q.Head.ptr.store(node->next.load());
            delete node;
        }
    }

    void enqueue(T value)
    {
        Node* node = new Node(value);
        while(true){
            Node* tailptr = Q.Tail.ptr.load();
            unsigned tailcount = Q.Tail.count.load();
            Node* next = tailptr->next.load();
            if(tailptr == Q.Tail.ptr.load() && tailcount == Q.Tail.count.load()){
                if(next == nullptr){
                    if(tailptr->next.compare_exchange_weak(next,node)){
                        Q.Tail.ptr.compare_exchange_weak(tailptr,node);
                        Q.Tail.count.compare_exchange_weak(tailcount,tailcount+1);
                        printQueue();
                        return;
                    }
                }
                else{
                    Q.Tail.ptr.compare_exchange_weak(tailptr,next);
                    Q.Tail.count.compare_exchange_weak(tailcount,tailcount+1);
                }
            }

        }
    }

    // bool dequeue(T& pvalue) {
    //     while (true) {
    //         Node* headPtr = Q.Head.ptr.load();
    //         unsigned headCount = Q.Head.count.load();
    //         Node* tailPtr = Q.Tail.ptr.load();
    //         unsigned tailCount = Q.Tail.count.load();
    //         Node* next = headPtr->next.load();
    //         if (headPtr == Q.Head.ptr.load() && headCount == Q.Head.count.load()) {
    //             if (headPtr == tailPtr) {
    //                 if (next == nullptr) {
    //                     return false;
    //                 }
    //                 Q.Tail.ptr.compare_exchange_weak(tailPtr, next);
    //                 Q.Tail.count.compare_exchange_weak(tailCount, tailCount + 1);
    //             } else {
    //                 pvalue = next->value;
    //                 if (Q.Head.ptr.compare_exchange_weak(headPtr, next)) {
    //                     Q.Head.count.compare_exchange_weak(headCount, headCount + 1);
    //                     delete headPtr;
    //                     printQueue();
    //                     return true;
    //                 }
    //             }
    //         }
    //     }
    // }

    bool dequeue(T& pvalue){
        while(true){
            Node* headPtr = Q.Head.ptr.load();
            unsigned headCount = Q.Head.count.load();

            Node* tailPtr = Q.Tail.ptr.load();
            unsigned tailCount = Q.Tail.count.load();

            Node* next = headPtr->next.load();

            if(headPtr==Q.Head.ptr.load() && headCount == Q.Head.count.load()){
                if(headPtr == tailPtr){
                    if(next == nullptr){
                        return false;
                    }
                    Q.Tail.ptr.compare_exchange_weak(tailPtr,next);
                    Q.Tail.count.compare_exchange_weak(tailCount,tailCount+1);
                }
                else{
                    pvalue = next->value;
                    if(Q.Head.ptr.compare_exchange_weak(headPtr,next)){
                        Q.Head.count.compare_exchange_weak(headCount,headCount+1);
                        delete headPtr;
                        printQueue();
                        return true;
                    }
                }
            }

        }
    }

    void printQueue() const {
        Node* current = Q.Head.ptr.load()->next.load();  // Skip dummy node
        std::cout << "Queue: ";
        while (current != nullptr) {
            std::cout << current->value << " ";
            current = current->next.load();
        }
        std::cout << std::endl;
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