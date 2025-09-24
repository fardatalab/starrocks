//
// Created by Felix Zhang on 2025-09-05.
//

#pragma once

#include <string>

#include "../ISocket.h"
#include "../utils/headers.h"

namespace fdl {

    class TCPSource final : public ISource {
    public:
        TCPSource() = default;
        ~TCPSource() override = default;

        void connect(const endpoint_t&& endpoint, job_id_t job, partition_id_t max_partition_id) override;
        void send(partition_id_t partition, const char* src, size_t size) override;
        void close() override;

    private:
        int sockfd_ = -1;
    };

}