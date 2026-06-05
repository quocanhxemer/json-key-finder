#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct PreparedKeys {
    std::vector<std::string> keys;
    std::vector<const uint8_t*> ptrs;
    std::vector<size_t> lens;
    std::vector<std::string_view> views;
};

/*
   Takes a list of keys, return a struct
   with data ready as arguments for APIs
*/
PreparedKeys prepare_keys(std::vector<std::string> keys);
