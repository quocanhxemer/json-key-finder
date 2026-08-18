#pragma once

#include "core/findkey_error.h"

#include <utility>

namespace teddy {

template <typename Function>
decltype(auto) dispatch_sigma(int sigma, Function&& function) {
    switch (sigma) {
        case 1:
            return std::forward<Function>(function).template operator()<1>();
        case 2:
            return std::forward<Function>(function).template operator()<2>();
        case 3:
            return std::forward<Function>(function).template operator()<3>();
        case 4:
            return std::forward<Function>(function).template operator()<4>();
        case 5:
            return std::forward<Function>(function).template operator()<5>();
        default:
            throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                               "Teddy suffix length is out of range");
    }
}

}  // namespace teddy
