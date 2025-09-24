//
// Created by Felix Zhang on 2025-09-08.
//

#pragma once
#include <cstdint>

#include "source.h"
#include "../ISocket.h"

namespace fdl {

    class TCPSink final : public ISink {
    public:
        TCPSink() = default;
        ~TCPSink() override = default;

        void connect(const endpoint_t&& endpoint, job_id_t job, partition_id_t max_partition_id) override;
        std::unordered_map<partition_id_t, std::pair<std::vector<char>, size_t>> receive(const std::vector<partition_id_t>&& partitions) override;

    private:
        int sockfd_ = -1;
    };

}