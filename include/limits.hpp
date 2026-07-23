#pragma once

struct Limits
{
    long long memory_bytes = 0;  // 0 = unlimited
    int cpu_percent = 0;         // 0 = unlimited
    int max_pids = 0;            // 0 = unlimited
};