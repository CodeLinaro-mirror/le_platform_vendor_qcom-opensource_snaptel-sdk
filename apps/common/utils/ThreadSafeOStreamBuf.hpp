/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ThreadSafeOStreamBuf.hpp
 *
 * @brief      Provides a custom stream buffer that wraps around std::cout to ensure
 *             thread-safe output in multi-threaded applications. This utility replaces
 *             the default stream buffer of std::cout with a mutex-protected version,
 *             preventing interleaved or corrupted output when multiple threads write
 *             concurrently.
 *
 *             The class overrides key streambuf methods:
 *             1. sync()      – Ensures flushing operations (e.g., std::endl, std::flush)
 *                              are serialized using a mutex.
 *             2. xsputn()    – Handles bulk output (e.g., std::cout << "text") safely.
 *             3. overflow()  – Handles single character output (e.g., std::cout << 'x')
 *                              with thread protection.
 *
 *             Upon construction, the class installs itself as the active stream buffer
 *             for std::cout. Upon destruction, it restores the original buffer.
 *
 *             Recommended for use in applications where std::cout is accessed from
 *             multiple threads and output consistency is critical.
 */

#ifndef THREADSAFEOSTREAMBUF_HPP
#define THREADSAFEOSTREAMBUF_HPP
#include <iostream>
#include <mutex>
#include <streambuf>

class ThreadSafeOStreamBuf : public std::streambuf {
    std::streambuf *original;
    std::mutex mtx;

 protected:
    // Called when flushing (endl, flush, ~ostream)
    int sync() override {
        std::lock_guard<std::mutex> lock(mtx);
        return original->pubsync();
    }

    // Called for bulk output (e.g. cout << "hello")
    std::streamsize xsputn(const char *s, std::streamsize count) override {
        std::lock_guard<std::mutex> lock(mtx);
        return original->sputn(s, count);
    }

    // Called for single char output (cout << 'x')
    int_type overflow(int_type ch) override {
        std::lock_guard<std::mutex> lock(mtx);
        if (ch != traits_type::eof()) {
            char c = traits_type::to_char_type(ch);
            if (original->sputn(&c, 1) != 1)
                return traits_type::eof();
        }
        return ch;
    }

 public:
    ThreadSafeOStreamBuf() {
        original = std::cout.rdbuf(this);  // redirect cout
    }

    ~ThreadSafeOStreamBuf() {
        std::cout.rdbuf(original);  // restore on exit
    }
};

#endif  // THREADSAFEOSTREAMBUF_HPP
