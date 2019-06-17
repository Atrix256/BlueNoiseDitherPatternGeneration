#pragma once

#include <vector>

// The algorithm from the forced random sampling paper
// http://drivenbynostalgia.com/#frs
void GenerateBN_FRS(
    std::vector<uint8_t>& blueNoise,
    size_t width,
    bool makeBlueNoise // if false, makes red noise
);