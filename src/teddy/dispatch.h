#pragma once

#include "core/findkey_error.h"

namespace teddy {

template <typename Result, typename Fn>
Result dispatch_sigma(int sigma, Fn&& fn) {
    switch (sigma) {
        case 1:
            return fn.template operator()<1>();
        case 2:
            return fn.template operator()<2>();
        case 3:
            return fn.template operator()<3>();
        case 4:
            return fn.template operator()<4>();
        case 5:
            return fn.template operator()<5>();
        default:
            throw FindkeyError(FindkeyErrorCode::INVALID_ARGUMENT,
                               "Teddy suffix length is out of range");
    }
}

}  // namespace teddy
