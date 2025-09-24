//
// Created by Felix Zhang on 2025-09-08.
//

#include <cassert>
#include <ranges>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "sink.h"
#include "../utils/constants.h"
#include "../utils/debug.h"
#include "../utils/headers.h"
#include "../utils/socket.h"

namespace fdl {

void TCPSink::connect(const endpoint_t&& endpoint, const job_id_t job, const partition_id_t max_partition_id) {
    assert(endpoint.protocol == TCP);
    auto& [ip, port] = endpoint.target.addr;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    sockfd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    ::connect(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    const header::JobMetadata metadata{
        .kind = header::JobMetadata::SINK,
        .job_id = job,
        .max_partition_id = max_partition_id,
    };
    send_all(sockfd_, &metadata, sizeof(metadata));
}

std::unordered_map<partition_id_t, std::pair<std::vector<char>, size_t>> TCPSink::receive(const std::vector<partition_id_t>&& partitions) {
    const header::SinkRequest request {
        .num_partitions = static_cast<uint32_t>(partitions.size())
    };
    send_all(sockfd_, &request, sizeof(request));
    send_all(sockfd_, partitions.data(), partitions.size() * sizeof(partition_id_t));

    std::unordered_map<partition_id_t, std::pair<std::vector<char>, size_t>> buffers;
    buffers.reserve(partitions.size());
    for (const auto& partition : partitions) {
        std::vector<char> buffer;
        buffer.reserve(1 * KiB);
        buffers.insert({partition, {std::move(buffer), 0}});
    }

    header::SinkResponse response {
        .done = false,
        .partition = 0,
        .size = 0
    };
    while (true) {
        recv_all(sockfd_, &response, sizeof(response));
        if (response.done) { break; }
        assert(buffers.contains(response.partition));

        auto& [buffer, offset] = buffers[response.partition];
        if (offset + response.size > buffer.size()) {
            buffer.resize(std::max(buffer.size() << 1, offset + response.size));
        }
        recv_all(sockfd_, buffer.data() + offset, response.size);
        offset += response.size;
    }

    return buffers;
}

}