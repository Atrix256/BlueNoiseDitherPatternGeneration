#pragma once

#include <vector>

// generates blue noise by repeatedly high pass filtering white noise and fixing up the histogram
// https://blog.demofox.org/2017/10/25/transmuting-white-noise-to-blue-red-green-purple/
void GenerateBN_HPF(std::vector<uint8_t>& blueNoise, size_t width, size_t numPasses = 5, float sigma = 1.0f, bool makeRed = false);
