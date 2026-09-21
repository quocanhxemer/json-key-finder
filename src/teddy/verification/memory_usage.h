#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

// Approximates memory usage of Teddy verifiers
namespace teddy::verification::detail {

template <typename T, typename Allocator>
size_t vector_allocation_bytes(
    const std::vector<T, Allocator>& values) noexcept {
    return values.capacity() * sizeof(T);
}

template <typename Key,
          typename Value,
          typename Hash,
          typename KeyEqual,
          typename Allocator>
size_t unordered_map_allocation_bytes(
    const std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>&
        values) noexcept {
    constexpr size_t node_overhead = sizeof(void*) + sizeof(size_t);
    const size_t buckets =
        values.empty() ? 0 : values.bucket_count() * sizeof(void*);
    const size_t nodes =
        values.size() *
        (sizeof(typename std::unordered_map<Key, Value, Hash, KeyEqual,
                                            Allocator>::value_type) +
         node_overhead);
    return buckets + nodes;
}

}  // namespace teddy::verification::detail
