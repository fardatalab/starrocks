//
// Created by Felix Zhang on 2025-09-04.
//

#pragma once

#include <new>

#include "common/compiler_util.h"

namespace fdl {

constexpr size_t KiB = 1024;
constexpr size_t MiB = 1024 * KiB;
constexpr size_t GiB = 1024 * MiB;

constexpr size_t STREAM_BUFFER_SIZE = 8 * MiB;

constexpr size_t PARTITION_BUFFER_SIZE = 1 * MiB;
constexpr size_t PARTITION_BUFFER_NUMBER = 128;

constexpr size_t MEMORY_BUFFER_SIZE = (1 * GiB) >> 1;
constexpr size_t MEMORY_BUFFER_NUMBER = 8;

constexpr size_t BACKEND_BATCH_SIZE = 4 * KiB;

} // namespace fdl
