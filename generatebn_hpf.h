#pragma once

#include <vector>

// generates blue noise by repeatedly high pass filtering white noise and fixing up the histogram
void GenerateBN_HPF(std::vector<uint8_t>& blueNoise, size_t width, size_t numPasses = 5, float sigma = 1.0f);
