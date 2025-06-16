#pragma once

#include <cstddef>

template<typename T, size_t sz>
size_t size(T(&)[sz]) {
    return sz;
}
