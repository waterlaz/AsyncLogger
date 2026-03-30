/* Copyright (c) 2026 Evgeniy Vodolazskiy (waterlaz)  */

#pragma once

#include <thread>
#include <pthread.h>
#include <atomic>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

template <typename T>
class RingBuffer {
private:
    alignas(64) std::atomic<size_t> head = 0;
    alignas(64) std::atomic<size_t> tail = 0;
    // buffer of size capacity
    std::vector<T> buffer;
public:
    bool pop(T& value) {
        size_t h = head.load(std::memory_order_relaxed);
        if (h == tail.load(std::memory_order_acquire)) {
            // the buffer is empty, no item to pop
            return false;
        }
        value = std::move(buffer[h]);
        head.store((h + 1) % buffer.size(), std::memory_order_release);
        return true;
    }
    bool push(const T& value) {
        size_t t = tail.load(std::memory_order_relaxed);
        size_t next = (t + 1) % buffer.size();
        if (next == head.load(std::memory_order_acquire)) {
            // the buffer is full, cannot push
            return false;
        }
        buffer[t] = value;
        tail.store(next, std::memory_order_release);
        return true;
    }
    RingBuffer(size_t capacity = 128) : buffer(capacity) {}
};

class AsyncLogger : std::basic_streambuf<char>, public std::ostream {
private:
    std::stringstream stringBuilder;
    std::ofstream file;
    RingBuffer<std::string> buffer;
    std::thread thread;
    bool isRunning = true;
    bool isRealTime = true;
    std::chrono::milliseconds flushInterval;
    void flushBuffer() {
        std::string line;
        while(buffer.pop(line)) {
            file << line;
        }
        file.flush();
    }
    void run() {
        while(isRunning) {
            flushBuffer();
            std::this_thread::sleep_for(flushInterval);
        }
        flushBuffer();
    }
    void disableRealTime(){
        struct sched_param param;
        param.sched_priority = 0;
        // Set the priority of the thread to non-realtime
        int err = pthread_setschedparam(thread.native_handle(),
            SCHED_OTHER, &param);
        if (err == 0) {
            // Successfully set the thread to non-realtime
            isRealTime = false;
        }
    }
public:
    AsyncLogger(
        const std::string& filename,
        size_t capacity = 1024,
        std::chrono::milliseconds flushInterval = std::chrono::milliseconds(100))
        : std::ostream(this), file(filename), buffer(capacity),
          flushInterval(flushInterval)
    {
        thread = std::thread(&AsyncLogger::run, this);
        disableRealTime();
    }
    ~AsyncLogger() {
        sync();
        isRunning = false;
        thread.join();
    }
    void log(const std::string& value) {
        buffer.push(value);
    }
    int overflow(int c) override {
        stringBuilder.put(c);
        return 0;
    }
    int sync() {
        std::string s = stringBuilder.str();
        if (!s.empty()) {
            log(s);
            stringBuilder.str("");
        }
        return 0;
    }
    bool isLoggerThreadRealTime() const {
        return isRealTime;
    }
};
