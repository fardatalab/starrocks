//
// Created by Felix Zhang on 2025-09-08.
//

#pragma once

#include <utility>

template<typename T>
void
hash_combine(std::size_t& seed, T const& key) {
    std::hash<T> hasher;
    seed ^= hasher(key) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<typename T1, typename T2>
struct std::hash<std::pair<T1, T2>> {
    std::size_t operator()(std::pair<T1, T2> const& pair) const {
        std::size_t seed(0);
        ::hash_combine(seed, pair.first);
        ::hash_combine(seed, pair.second);
        return seed;
    }
};
