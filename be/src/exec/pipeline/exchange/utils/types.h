//
// Created by Felix Zhang on 2025-09-17.
//

#pragma once

#include <cstdint>

#include "constants.h"

namespace fdl {

template <typename T>
struct alignas(CACHE_LINE_SIZE) CacheAligned {
    static_assert(sizeof(T) < CACHE_LINE_SIZE, "Type must be smaller than CACHE_LINE_SIZE");
    T value;
    char pad[CACHE_LINE_SIZE - sizeof(T)];
};

using job_id_t = uint32_t;
using partition_id_t = uint32_t;

} // namespace fdl
