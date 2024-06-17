#ifndef IQUEUE_H
#define IQUEUE_H

template <typename T>
class IQueue {
public:
    virtual ~IQueue() = default;
    virtual void enqueue(T value) = 0;
    virtual bool dequeue(T& value) = 0;
};

#endif // IQUEUE_H