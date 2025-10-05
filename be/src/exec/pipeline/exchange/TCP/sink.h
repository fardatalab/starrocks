//
// Created by Felix Zhang on 2025-09-08.
//

#pragma once

#include <optional>

#include "source.h"
#include "../ISocket.h"

namespace fdl {

    class TCPSink final : public ISink {
    public:
        TCPSink() = default;
        ~TCPSink() override = default;

        void connect(const endpoint_t&& endpoint, job_id_t job, partition_id_t max_partition_id) override;
        void receive(std::vector<partition_id_t>&& partitions, partition_map_t* buffers) override;
        void consume(partition_map_t *buffers) override;

    private:
        int sockfd_ = -1;
        std::optional<std::vector<partition_id_t>> partitions_ = std::nullopt;
    };

}