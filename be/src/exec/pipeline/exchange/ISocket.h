#pragma once

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "utils/headers.h"

namespace fdl {

enum Protocol {
    TCP = 0,
    IB = 1,
};

struct endpoint_t {
    Protocol protocol;
    union {
        std::pair<const char*, uint16_t> addr;
    } target;
};

class ISource {
public:
    virtual ~ISource() {};

    virtual void connect(const endpoint_t&& endpoint, job_id_t job, partition_id_t max_partition_id) = 0;
    virtual void send(partition_id_t partition, const char* src, size_t size) = 0;
    virtual void close() = 0;
};

class ISink {
public:
    virtual ~ISink() {};

    virtual void connect(const endpoint_t&& endpoint, job_id_t job, partition_id_t max_partition_id) = 0;
    virtual std::unordered_map<partition_id_t, std::pair<std::vector<char>, size_t>> receive(const std::vector<partition_id_t>&& partitions) = 0;
};

}