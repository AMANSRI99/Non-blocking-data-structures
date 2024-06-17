# Non-Blocking Queue Benchmarking

This repository contains a C++ implementation of a non-blocking queue and includes a benchmarking tool to evaluate its performance. Follow the instructions below to compile and run the benchmark.

## Prerequisites

Ensure you have a C++14 compatible compiler installed on your system.

## Running the Benchmark

1. Navigate to the `cpp` directory of the repository.

2. Compile the benchmarking tool using the following command:
    ```shell
    g++ -std=c++14 -o benchmark benchmarking.cpp
    ```

3. Run the benchmark with the following command:
    ```shell
    ./benchmark <number of elements> <number of producer threads> <number of consumer threads> <0 for non blocking and 1 for blocking>
    ```

### Example

To run a benchmark with 10000 elements, 4 producer threads, and 4 consumer threads, use:
```shell
./benchmark 10000 4 4 0
```


#Non-Blocking Queue Benchmarking

Follow the same above steps with benchmarking_with_core_affinity.cpp instead of benchmarking.cpp

Note - There's no real difference observed with both the benchmarkings.
