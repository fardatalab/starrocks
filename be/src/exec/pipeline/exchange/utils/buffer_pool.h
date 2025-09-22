//
// Created by Felix Zhang on 2025-09-10.
//

#pragma once

#include <array>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <queue>

#include "linear_buffer.h"
#include "ring_buffer.h"

namespace fdl {

using PartitionBuffer = common::RingBuffer<PARTITION_BUFFER_SIZE>;
using MemoryBuffer = common::LinearBuffer<MEMORY_BUFFER_SIZE>;

}

namespace fdl::common {

template<typename T, std::size_t N>
class BufferPool {
public:
    BufferPool() {
        for (size_t i = 0; i < N; ++i) { free_.push(&buffers_[i]); }
    }

    T* acquire() {
        std::unique_lock lock(mutex_);

        if (free_.empty()) {
            cv_.wait(lock, [&] { return !free_.empty(); });
        }

        auto buffer_ = free_.front();
        free_.pop();
        return buffer_;
    }

    std::vector<T*> acquire(size_t n) {
        assert(n <= N);
        std::unique_lock lock(mutex_);

        if (free_.size() < n) {
            cv_.wait(lock, [&] { return free_.size() >= n; });
        }

        std::vector<T*> buffers(n);
        for (size_t i = 0; i < n; ++i) {
            buffers[i] = free_.front();
            free_.pop();
        }
        return buffers;
    }

    void release(T* index) {
        {
            std::scoped_lock lock(mutex_);
            free_.push(index);
        }
        cv_.notify_one();
    }

    void release(std::vector<T*> indexes) {
        {
            std::scoped_lock lock(mutex_);
            for (auto index : indexes) {
                free_.push(index);
            }
        }
        cv_.notify_one();
    }

private:
    std::array<T, N> buffers_;

    std::queue<T*> free_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

struct BufferPools {
    // Used by the frontend to store incoming data for each partition
    static BufferPool<PartitionBuffer, PARTITION_BUFFER_NUMBER>& partition() {
        static BufferPool<PartitionBuffer, PARTITION_BUFFER_NUMBER> pool;
        return pool;
    };

    // Used by the memory backend to materialize partition data
    static BufferPool<MemoryBuffer, MEMORY_BUFFER_NUMBER>& memory() {
        static BufferPool<MemoryBuffer, MEMORY_BUFFER_NUMBER> pool;
        return pool;
    }
};

}
