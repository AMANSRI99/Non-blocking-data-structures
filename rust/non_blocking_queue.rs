use std::sync::atomic::{AtomicPtr, AtomicUsize, Ordering};
use std::ptr;
use std::fmt::Debug;

struct Node<T> {
    value: T,
    next: AtomicPtr<Node<T>>,
}

impl<T> Node<T> {
    fn new(value: T) -> *mut Self {
        Box::into_raw(Box::new(Node {
            value,
            next: AtomicPtr::new(ptr::null_mut()),
        }))
    }
}

struct Pointer<T> {
    ptr: AtomicPtr<Node<T>>,
    count: AtomicUsize,
}

impl<T> Pointer<T> {
    fn new(node: *mut Node<T>, count: usize) -> Self {
        Pointer {
            ptr: AtomicPtr::new(node),
            count: AtomicUsize::new(count),
        }
    }
}

struct Queue<T> {
    head: Pointer<T>,
    tail: Pointer<T>,
}

impl<T> Queue<T> {
    fn new() -> Self {
        let dummy = Node::new(unsafe { std::mem::zeroed() });
        Queue {
            head: Pointer::new(dummy, 0),
            tail: Pointer::new(dummy, 0),
        }
    }
}

pub struct LockFreeQueue<T: Clone + Debug> {
    q: Queue<T>,
}

impl<T: Clone + Debug> LockFreeQueue<T> {
    pub fn new() -> Self {
        LockFreeQueue {
            q: Queue::new(),
        }
    }

    pub fn enqueue(&self, value: T) {
        let node = Node::new(value);
        loop {
            let tail_ptr = self.q.tail.ptr.load(Ordering::Acquire);
            let tail_count = self.q.tail.count.load(Ordering::Acquire);
            let next = unsafe { (*tail_ptr).next.load(Ordering::Acquire) };

            if tail_ptr == self.q.tail.ptr.load(Ordering::Acquire) && tail_count == self.q.tail.count.load(Ordering::Acquire) {
                if next.is_null() {
                    if unsafe { (*tail_ptr).next.compare_exchange(next, node, Ordering::Release, Ordering::Relaxed).is_ok() } {
                        self.q.tail.ptr.compare_exchange(tail_ptr, node, Ordering::Release, Ordering::Relaxed).ok();
                        self.q.tail.count.compare_exchange(tail_count, tail_count + 1, Ordering::Release, Ordering::Relaxed).ok();
                        self.print_queue();
                        return;
                    }
                } else {
                    self.q.tail.ptr.compare_exchange(tail_ptr, next, Ordering::Release, Ordering::Relaxed).ok();
                    self.q.tail.count.compare_exchange(tail_count, tail_count + 1, Ordering::Release, Ordering::Relaxed).ok();
                }
            }
        }
    }

    pub fn dequeue(&self, pvalue: &mut T) -> bool {
        loop {
            let head_ptr = self.q.head.ptr.load(Ordering::Acquire);
            let head_count = self.q.head.count.load(Ordering::Acquire);

            let tail_ptr = self.q.tail.ptr.load(Ordering::Acquire);
            let tail_count = self.q.tail.count.load(Ordering::Acquire);

            let next = unsafe { (*head_ptr).next.load(Ordering::Acquire) };

            if head_ptr == self.q.head.ptr.load(Ordering::Acquire) && head_count == self.q.head.count.load(Ordering::Acquire) {
                if head_ptr == tail_ptr {
                    if next.is_null() {
                        return false;
                    }
                    self.q.tail.ptr.compare_exchange(tail_ptr, next, Ordering::Release, Ordering::Relaxed).ok();
                    self.q.tail.count.compare_exchange(tail_count, tail_count + 1, Ordering::Release, Ordering::Relaxed).ok();
                } else {
                    *pvalue = unsafe { (*next).value.clone() };
                    if self.q.head.ptr.compare_exchange(head_ptr, next, Ordering::Release, Ordering::Relaxed).is_ok() {
                        self.q.head.count.compare_exchange(head_count, head_count + 1, Ordering::Release, Ordering::Relaxed).ok();
                        unsafe { drop(Box::from_raw(head_ptr)) }; // Free the old head node
                        self.print_queue();
                        return true;
                    }
                }
            }
        }
    }

    pub fn print_queue(&self) {
        unsafe {
            let mut current = self.q.head.ptr.load(Ordering::Acquire).as_ref().unwrap().next.load(Ordering::Acquire);
            print!("Queue: ");
            while let Some(node) = current.as_ref() {
                print!("{:?} ", node.value);
                current = node.next.load(Ordering::Acquire);
            }
            println!();
        }
    }
}

fn main() {
    let queue = LockFreeQueue::new();

    // Example usage:
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);

    let mut value = 0;
    if queue.dequeue(&mut value) {
        println!("Dequeued: {}", value);
    }

    if queue.dequeue(&mut value) {
        println!("Dequeued: {}", value);
    }

    if queue.dequeue(&mut value) {
        println!("Dequeued: {}", value);
    }
}