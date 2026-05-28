#pragma once

template <typename Result, typename Fn>
Result dispatch_teddy_sigma(int sigma, Fn&& fn) {
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
            return Result{};
    }
}
