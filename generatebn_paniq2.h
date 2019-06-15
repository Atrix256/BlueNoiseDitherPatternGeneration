#pragma once

#include <vector>

// CPU implementation of his shadertoy, uses Martin Roberts R1 sequence on a hilbert curve
// https://www.shadertoy.com/view/3tB3z3
void GenerateBN_Paniq2(
    std::vector<uint8_t>& blueNoise,
    size_t width
);