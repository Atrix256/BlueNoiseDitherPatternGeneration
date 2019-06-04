#include "generatebn_void_cluster.h"
#include "whitenoise.h"

void GenerateBN_Void_Cluster(
    std::vector<uint8_t>& blueNoise,
    size_t width
)
{
    std::mt19937 rng(GetRNGSeed());
    std::uniform_int_distribution<size_t> dist(0, width*width);

    // make the initial binary pattern
    std::vector<bool> binaryPattern(width*width, false);
    size_t ones = width * width / 2;
    for (size_t index = 0; index < width; ++index)
    {
        size_t pixel = dist(rng);
        binaryPattern[pixel] = true;
    }


    std::vector<size_t> ranks(width*width, 0);
}