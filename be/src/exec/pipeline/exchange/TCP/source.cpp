//
// Created by Felix Zhang on 2025-09-05.
//

#include <cassert>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "source.h"
#include "../utils/socket.h"

namespace fdl {

    void TCPSource::connect(const endpoint_t&& endpoint, const job_id_t job, const partition_id_t max_partition_id) {
        assert(endpoint.protocol == TCP);
        auto& [ip, port] = endpoint.target.addr;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &addr.sin_addr);
        addr.sin_port = htons(port);

        sockfd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        ::connect(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        const header::JobMetadata metadata{
            .kind = header::JobMetadata::SOURCE,
            .job_id = job,
            .max_partition_id = max_partition_id,
        };
        send_all(sockfd_, &metadata, sizeof(metadata));
    }

    void TCPSource::send(const partition_id_t partition, const char* src, const size_t size) {
        const header::Source header {
            .done = false,
            .partition = partition,
            .size = static_cast<uint32_t>(size)
        };
        send_all(sockfd_, &header, sizeof(header));
        send_all(sockfd_, src, size);
    }

    void TCPSource::close() {
        header::Source header{};
        header.done = true;
        send_all(sockfd_, &header, sizeof(header));
    }

}