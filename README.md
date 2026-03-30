# AsyncLogger

A lightweight, header-only C++ class for non-blocking, asynchronous logging — designed for use in real-time and latency-sensitive applications.

## Features

- **Header-only** — a single `#include` is all you need.
- **Non-blocking writes** — log messages are pushed into a lock-free ring buffer; the calling thread is never stalled waiting for disk I/O.
- **Background flush thread** — a dedicated thread drains the ring buffer to a file at a configurable interval.
- **Real-time friendly** — the background thread is automatically demoted to the standard `SCHED_OTHER` scheduling policy so it does not compete with real-time threads.
- **`std::ostream` interface** — use the familiar `<<` operator to compose and emit log lines.

## Requirements

- C++17 (or later)
- POSIX threads (`pthreads`) — used for thread scheduling control
- Linux (the `pthread_setschedparam` call for non-realtime demotion is Linux-specific)
- A C++ compiler such as `g++` or `clang++`

## Installation

`AsyncLogger` is header-only. Copy `AsyncLogger.hpp` into your project and include it:

```cpp
#include "AsyncLogger.hpp"
```

No build system integration or linking step is required beyond linking pthreads (`-lpthread`).

## API Reference

### `AsyncLogger`

```cpp
AsyncLogger(
    const std::string& filename,
    size_t capacity = 1024,
    std::chrono::milliseconds flushInterval = std::chrono::milliseconds(100)
);
```

| Parameter       | Default  | Description                                                                 |
|-----------------|----------|-----------------------------------------------------------------------------|
| `filename`      | —        | Path to the output log file. Created or overwritten on construction.        |
| `capacity`      | `1024`   | Maximum number of log messages that can be queued in the ring buffer at once. Pushes to a full buffer are silently dropped. |
| `flushInterval` | `100 ms` | How often the background thread wakes up to flush queued messages to disk.  |

The destructor flushes any remaining messages and joins the background thread, so all messages logged before the `AsyncLogger` object goes out of scope are guaranteed to be written.

---

#### `void log(const std::string& message)`

Push a pre-built string directly onto the ring buffer.

```cpp
logger.log("system started\n");
```

#### `operator<<`

`AsyncLogger` inherits from `std::ostream`, so you can stream values to it exactly as you would to `std::cout` or `std::cerr`. Flushing (via `std::endl` or `std::flush`) commits the buffered characters as a single entry in the ring buffer.

```cpp
logger << "temperature=" << 42.5 << "\n";   // buffered, flushed on next sync
logger << "event triggered" << std::endl;   // flushed immediately
```

#### `int isLoggerThreadRealTime() const`

Returns `1` if the background flush thread is still running at a real-time scheduling priority, `0` if it has been successfully demoted to `SCHED_OTHER`. Under normal operation this returns `0`.

```cpp
if (logger.isLoggerThreadRealTime()) {
    // unexpected — demotion failed
}
```

---

### `RingBuffer<T>`

A single-producer / single-consumer lock-free ring buffer that `AsyncLogger` uses internally. It is also available for direct use if you need a generic concurrent queue.

```cpp
RingBuffer<int> rb(/*capacity=*/128);

rb.push(42);   // returns false if the buffer is full

int value;
rb.pop(value); // returns false if the buffer is empty
```

## Usage Examples

### Basic usage

```cpp
#include "AsyncLogger.hpp"

int main() {
    // Open "app.log", default capacity (1024) and flush interval (100 ms)
    AsyncLogger logger("app.log");

    logger << "Application started\n";
    logger << "Processing item " << 1 << std::endl;

    // logger destructor flushes remaining messages and stops the background thread
    return 0;
}
```

### Custom capacity and flush interval

```cpp
#include "AsyncLogger.hpp"
#include <chrono>

int main() {
    // Larger ring buffer and more frequent flushes
    AsyncLogger logger("app.log", 4096, std::chrono::milliseconds(10));

    for (int i = 0; i < 10000; ++i) {
        logger << "iteration " << i << "\n";
    }

    return 0;
}
```

### Using `log()` directly

```cpp
#include "AsyncLogger.hpp"
#include <string>

int main() {
    AsyncLogger logger("events.log");

    std::string msg = "sensor_value=3.14\n";
    logger.log(msg);

    return 0;
}
```

## Building and Running Tests

The test suite lives in the `tests/` directory and uses a plain `Makefile`.

```bash
cd tests
make all        # builds and runs both testRingBuffer and testLogger
make clean      # removes binaries and the test log file
```

Compile flags used by the tests:

```
g++ -O2 -mtune=native -Wall <test>.cpp -o <test> -lpthread
```

## Design Notes

- **Lock-free ring buffer** — `RingBuffer<T>` uses `std::atomic` head/tail indices with acquire/release memory ordering. It is safe for exactly one producer thread and one consumer thread.
- **Background thread** — on construction, a `std::thread` is started that loops: flush the ring buffer → sleep for `flushInterval` → repeat. On destruction the loop exits and a final flush is performed before `thread.join()` returns.
- **Real-time scheduling demotion** — immediately after the background thread is launched, `pthread_setschedparam` is called to move it to `SCHED_OTHER` priority 0. This ensures the logger thread does not compete for CPU time with any real-time threads in your application.
- **Dropped messages** — if the ring buffer is full (the background thread cannot drain it fast enough), `push()` returns `false` and the message is silently dropped. Increase `capacity` or decrease `flushInterval` if you observe message loss.

## License

See [LICENSE](LICENSE).
