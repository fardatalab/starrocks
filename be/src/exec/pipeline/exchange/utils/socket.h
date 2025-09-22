//
// Created by Felix Zhang on 2025-09-18.
//

#pragma once

#include <sys/socket.h>

namespace fdl {

inline bool send_all(const int sockfd, const void* buffer, size_t size) {
    auto data = static_cast<const char*>(buffer);
    while (size > 0) {
        const ssize_t n = ::send(sockfd, data, size, 0);
        if (n < 0) {
            perror("send_all");
            return false;
        }

        data += n;
        size -= n;
    }
    return true;
}

inline bool recv_all(const int sockfd, void* buffer, size_t size) {
    auto data = static_cast<char*>(buffer);
    while (size > 0) {
        const ssize_t n = ::recv(sockfd, data, size, 0);
        if (n <= 0) {
            if (n < 0) { perror("recv_all"); }
            return false;
        }
        data += n;
        size -= n;
    }
    return true;
}

}
