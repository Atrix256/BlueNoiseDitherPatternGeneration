#pragma once

#include <chrono>

struct ScopedTimer
{
    ScopedTimer(const char* label)
    {
        m_start = std::chrono::high_resolution_clock::now();
        printf("%s...\n", label);
    }

    ~ScopedTimer()
    {
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - m_start);
        printf("%f ms\n\n", time_span.count() * 1000.0f);
    }

    std::chrono::high_resolution_clock::time_point m_start;
};