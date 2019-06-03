#pragma once

#include <vector>

// generates blue noise by swapping white noise pixels that make it more blue
// https://www.arnoldrenderer.com/research/dither_abstract.pdf
void GenerateBN_Swap(
    std::vector<uint8_t>& blueNoise,
    size_t width,
    size_t swapTries,
    const char* csvFileName,
    bool limitTo3Sigma,
    float simulatedAnnealingCoolingMultiplier,
    int numSimultaneousSwaps
);
