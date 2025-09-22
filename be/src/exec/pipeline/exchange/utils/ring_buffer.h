///
/// Created by Felix Zhang on 2025-09-04.
///
/// This was adapted from DDS [VLDB '24]
///

#pragma once

#include <atomic>
#include <cstring>

#include "constants.h"
#include "types.h"

namespace fdl::common {

struct ConsumeResult {
    char *buffer_1 = nullptr;
    char *buffer_2 = nullptr;
    size_t size_1 = 0;
    size_t size_2 = 0;
};

template<
    std::size_t N,
    std::size_t M = (N >> 3)
>
class RingBuffer {
public:
    RingBuffer() { buffer_ = new char[N]; };

    /// @brief Produce data into the ring buffer. Note the data is copied
    /// @param data pointer to data to be copied
    /// @param size size of the data to be copied
    /// @return true if success, false if failure
    bool produce(const char *data, const size_t size) {
        auto producer_idx = producer_index_.value.load(std::memory_order_relaxed);
        auto consumer_idx = consumer_index_.value;

        size_t distance = 0;
        if (producer_idx < consumer_idx) {
            distance = consumer_idx + BUFFER_SIZE - producer_idx;
        } else {
            distance = producer_idx - consumer_idx;
        }

        size_t request_bytes = sizeof(size_t) + size;
        if (request_bytes % sizeof(size_t) != 0) {
            request_bytes += sizeof(size_t) - request_bytes % sizeof(size_t);
        }

        if (distance + request_bytes >= MAX_PRODUCER_ADVANCEMENT) {
            return false;
        }
        if (request_bytes > BUFFER_SIZE - distance) {
            return false;
        }

        // Update the producer index
        while (!producer_index_.value.compare_exchange_weak(
            producer_idx, (producer_idx + request_bytes) % BUFFER_SIZE)) {
            producer_idx = producer_index_.value.load(std::memory_order_relaxed);
            consumer_idx = consumer_index_.value;

            if (producer_idx <= consumer_idx) {
                distance = consumer_idx + BUFFER_SIZE - producer_idx;
            } else {
                distance = producer_idx - consumer_idx;
            }
            if (distance + request_bytes >= MAX_PRODUCER_ADVANCEMENT) {
                return false;
            }
            if (request_bytes > BUFFER_SIZE - distance) {
                return false;
            }
        }

        if (producer_idx + sizeof(size_t) + size <= BUFFER_SIZE) {
            char *req_addr = &buffer_[producer_idx];
            reinterpret_cast<size_t *>(req_addr)[0] = size;
            std::memcpy(req_addr + sizeof(size_t), data, size);

            // Update progress
            auto progress = progress_.value.load(std::memory_order_relaxed);
            while (!progress_.value.
                compare_exchange_weak(progress, (progress + request_bytes) % BUFFER_SIZE)) {
                progress = progress_.value.load(std::memory_order_relaxed);
            }
        } else {
            // Split the request into two parts
            const size_t remaining_bytes = BUFFER_SIZE - producer_idx - sizeof(size_t);
            char *req_addr = &buffer_[producer_idx];
            char *req_addr_2 = &buffer_[0];

            // Write the number of bytes in
            reinterpret_cast<size_t *>(req_addr)[0] = size;
            if (remaining_bytes > 0) {
                std::memcpy(req_addr + sizeof(size_t), data, remaining_bytes);
            }
            std::memcpy(req_addr_2, data + remaining_bytes, size - remaining_bytes);

            // Update progress
            auto progress = progress_.value.load(std::memory_order_relaxed);
            while (!progress_.value.
                compare_exchange_weak(progress, (progress + request_bytes) % BUFFER_SIZE)) {
                progress = progress_.value.load(std::memory_order_relaxed);
            }
        }

        return true;
    }

    /// @brief Produce two buffers into the ring buffer as one request. Note the data is copied
    /// @param data1 pointer to first buffer to be copied
    /// @param data2 pointer to second buffer to be copied
    /// @param size1 size of the first buffer to be copied
    /// @param size2 size of the second buffer to be copied
    /// @return true if success, false if failure
    bool produce(const char *data1, const char *data2, const size_t size1, const size_t size2) {
        auto producer_idx = producer_index_.value.load(std::memory_order_relaxed);
        auto consumer_idx = consumer_index_.value;

        size_t distance = 0;
        const auto combined_size = size1 + size2;
        if (producer_idx < consumer_idx) {
            distance = consumer_idx + BUFFER_SIZE - producer_idx;
        } else {
            distance = producer_idx - consumer_idx;
        }

        size_t request_bytes = sizeof(size_t) + combined_size;
        if (request_bytes % sizeof(size_t) != 0) {
            request_bytes += sizeof(size_t) - request_bytes % sizeof(size_t);
        }

        if (distance + request_bytes >= MAX_PRODUCER_ADVANCEMENT) {
            return false;
        }
        if (request_bytes > BUFFER_SIZE - distance) {
            return false;
        }

        // Update the producer index
        while (!producer_index_.value.compare_exchange_weak(
            producer_idx, (producer_idx + request_bytes) % BUFFER_SIZE)) {
            producer_idx = producer_index_.value.load(std::memory_order_relaxed);
            consumer_idx = consumer_index_.value;

            if (producer_idx <= consumer_idx) {
                distance = consumer_idx + BUFFER_SIZE - producer_idx;
            } else {
                distance = producer_idx - consumer_idx;
            }
            if (distance + request_bytes >= MAX_PRODUCER_ADVANCEMENT) {
                return false;
            }
            if (request_bytes > BUFFER_SIZE - distance) {
                return false;
            }
        }

        if (producer_idx + sizeof(size_t) + combined_size <= BUFFER_SIZE) {
            char *req_addr = &buffer_[producer_idx];
            reinterpret_cast<size_t *>(req_addr)[0] = combined_size;
            std::memcpy(req_addr + sizeof(size_t), data1, size1);
            std::memcpy(req_addr + sizeof(size_t) + size1, data2, size2);

            // Update progress
            auto progress = progress_.value.load(std::memory_order_relaxed);
            while (!progress_.value.
                compare_exchange_weak(progress, (progress + request_bytes) % BUFFER_SIZE)) {
                progress = progress_.value.load(std::memory_order_relaxed);
            }
        } else {
            // Split the request into two parts
            const size_t remaining_bytes = BUFFER_SIZE - producer_idx - sizeof(size_t);
            char *req_addr = &buffer_[producer_idx];
            char *req_addr_2 = &buffer_[0];

            // Write the buffers
            reinterpret_cast<size_t *>(req_addr)[0] = combined_size;
            if (remaining_bytes > 0) {
                if (remaining_bytes >= size1) {
                    std::memcpy(req_addr + sizeof(size_t), data1, size1);
                    if (remaining_bytes - size1 >= size2) {
                        std::memcpy(req_addr + sizeof(size_t) + size1, data2, remaining_bytes - size1);
                    } else {
                        std::memcpy(req_addr_2, data1 + size1, remaining_bytes - size1);
                    }
                } else {
                    std::memcpy(req_addr + sizeof(size_t), data1, remaining_bytes);
                    std::memcpy(req_addr_2, data1 + remaining_bytes, size1 - remaining_bytes);
                    std::memcpy(req_addr_2 + size1 - remaining_bytes, data2, size2);
                }
            } else {
                std::memcpy(req_addr_2, data1, size1);
                std::memcpy(req_addr_2 + size1, data2, size2);
            }

            // Update progress
            auto progress = progress_.value.load(std::memory_order_relaxed);
            while (!progress_.value.
                compare_exchange_weak(progress, (progress + request_bytes) % BUFFER_SIZE)) {
                progress = progress_.value.load(std::memory_order_relaxed);
            }
        }
        return true;
    }

    /// @brief Consume data from the ring buffer. Note the data is copied into the provided buffer
    /// @param data pointer to buffer to copy data into
    /// @param size size of the data to be copied
    /// @return true if success, false if failure
    bool consume(char *data, size_t &size) {
        // The following invariants are maintained on production into this queue:
        //   (1) tail is advanced
        //   (2) request is inserted
        //   (3) progress is incremented
        // The order of reading the progress pointer and the tail at the consumer matters
        // If the consumer reads the tail before progress, its possible the producer performs all three steps, updating progress
        // The reader is thus left with an invalid progress pointer.

        const auto progress = progress_.value.load(std::memory_order_relaxed);
        const auto producer_index = producer_index_.value.load(std::memory_order_acquire);
        const auto consumer_index = consumer_index_.value;

        if (consumer_index == producer_index) {
            // Nothing to consume
            return false;
        }

        if (progress != producer_index) {
            // The tail has been advanced, but the progress has not been updated
            return false;
        }

        // It is now safe to copy requests
        size_t available_bytes = 0;
        size_t output_size = 0;
        char *source_buffer_1 = nullptr;
        char *source_buffer_2 = nullptr;

        if (progress > consumer_index) {
            available_bytes = progress - consumer_index;
            output_size = available_bytes;
            source_buffer_1 = &buffer_[consumer_index];
            size = output_size;
        } else {
            available_bytes = BUFFER_SIZE - consumer_index;
            output_size = available_bytes + progress;
            source_buffer_1 = &buffer_[consumer_index];
            size = output_size;
            source_buffer_2 = &buffer_[0];
        }

        std::memcpy(data, source_buffer_1, available_bytes);
        memset(source_buffer_1, 0, available_bytes);

        if (source_buffer_2) {
            std::memcpy(data + available_bytes, source_buffer_2, size - available_bytes);
            memset(source_buffer_2, 0, size - available_bytes);
        }

        consumer_index_.value = progress;
        return true;
    }

    // Getters and setters
    char* buffer() const noexcept {
        return buffer_;
    };

    uint64_t producer_index() const noexcept {
        return producer_index_.value.load(std::memory_order_relaxed);
    }

    uint64_t progress() const noexcept {
        return progress_.value.load(std::memory_order_relaxed);
    }

    uint64_t consumer_index() const noexcept { return consumer_index_.value; }

    void producer_index(const uint64_t value) noexcept {
        producer_index_.value.store(value, std::memory_order_relaxed);
    }

    void progress(const uint64_t value) noexcept {
        progress_.value.store(value, std::memory_order_relaxed);
    }

    void consumer_index(const uint64_t value) noexcept {
        consumer_index_.value = value;
    }

    static constexpr size_t BUFFER_SIZE = N;
    static constexpr size_t MAX_PRODUCER_ADVANCEMENT = M;

private:
    CacheAligned<std::atomic<uint64_t> > producer_index_{0};
    CacheAligned<std::atomic<uint64_t> > progress_{0};
    CacheAligned<uint64_t> consumer_index_{0};

    static_assert((N > 1) & !(N & (N - 1)), "Buffer size must be a power of two");
    char* buffer_;
};

}
