#pragma once

#include <cerrno>
#include <system_error>

inline int checked(int ret, const char *msg){
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), msg);
    }
    return ret;
}