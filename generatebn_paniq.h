#pragma once

#include <vector>

// CPU implementation of his shadertoy, inspired by the swapping paper.
// https://www.shadertoy.com/view/XtdyW2
void GenerateBN_Paniq(
    std::vector<uint8_t>& blueNoise,
    size_t width,
    size_t iterations
);