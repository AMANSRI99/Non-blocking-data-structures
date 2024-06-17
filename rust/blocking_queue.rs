use std::sync::{Arc, Mutex, Condvar};
use std::collections::VecDeque;
use std::fmt::Debug;

#[derive(Clone)]
pub struct BlockingQueue<T: Debug> {
    queue: Arc<Mutex<VecDeque<T>>>,
    condvar: Arc<Condvar>,
    done: Arc<Mutex<bool>>,
}

impl<T: Debug> BlockingQueue<T> {
    pub fn new() -> Self {
        BlockingQueue {
            queue: Arc::new(Mutex::new(VecDeque::new())),
            condvar: Arc::new(Condvar::new()),
            done: Arc::new(Mutex::new(false)),
        }
    }

    pub fn enqueue(&self, value: T) {
        {
            let mut queue = self.queue.lock().unwrap();
            queue.push_back(value);
        }
        self.condvar.notify_one();
    }

    pub fn dequeue(&self) -> Option<T> {
        let mut queue = self.queue.lock().unwrap();
        loop {
            if *self.done.lock().unwrap() && queue.is_empty() {
                return None;
            }
            if let Some(value) = queue.pop_front() {
                return Some(value);
            }
            queue = self.condvar.wait(queue).unwrap();
        }
    }

    pub fn print_queue(&self) {
        let queue = self.queue.lock().unwrap();
        print!("Queue: ");
        for item in queue.iter() {
            print!("{:?} ", item);
        }
        println!();
    }

    pub fn set_done(&self) {
        {
            let mut done = self.done.lock().unwrap();
            *done = true;
        }
        self.condvar.notify_all();
    }
}

fn main() {
    let queue = BlockingQueue::new();

    // Example usage
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);

    queue.print_queue();

    let handle = std::thread::spawn({
        let queue = queue.clone();
        move || {
            while let Some(value) = queue.dequeue() {
                println!("Dequeued: {}", value);
            }
        }
    });

    queue.set_done();
    handle.join().unwrap();
}
