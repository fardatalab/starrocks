//
// Created by Felix Zhang on 2025-09-08.
//

#pragma once

#include <cstdint>

#include "types.h"

namespace fdl::header {

    struct JobMetadata {
        enum kind_t : uint8_t {
            SOURCE = 0,
            SINK = 1,
        } kind;
        job_id_t job_id;
        partition_id_t max_partition_id;
    };

    struct Source {
        bool is_done;
        partition_id_t partition;
        uint32_t size;
    };

    struct SinkRequest {
        uint32_t num_partitions;
    };

    struct SinkResponse {
        bool is_done;
        partition_id_t partition;
        uint32_t size;
    };

}