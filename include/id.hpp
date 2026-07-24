#pragma once

#include <cstdint>
#include <cstdio>
#include <random>
#include <string>

inline uint16_t make_id(){
    std::random_device rd;
    return static_cast<uint16_t>(rd() & 0xffff); //masked to 16 bits
}

// 0x3f9a -> "3f9a", zero-padded to 4 chars
inline std::string id_hex(uint16_t id)
{
    char buf[5];
    std::snprintf(buf, sizeof(buf), "%04x", id);
    return buf;
}