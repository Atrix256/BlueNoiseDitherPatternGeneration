#include "generatebn_void_cluster.h"
#include "whitenoise.h"

void MakeInitialBinaryPattern(std::vector<bool>& binaryPattern, size_t width)
{
    std::mt19937 rng(GetRNGSeed());
    std::uniform_int_distribution<size_t> dist(0, width*width);

    size_t ones = width * width / 2;
    for (size_t index = 0; index < width; ++index)
    {
        size_t pixel = dist(rng);
        binaryPattern[pixel] = true;
    }

    int offsetStart = -int(width) / 2;
    int offsetEnd = int(width) / 2;
    if ((width & 1) == 0)
        offsetEnd--;

    // TODO: multithread
    // TODO: limit to a 3 sigma search area

    //while (1)
    {
        for (int y = 0; y < width; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                for (int iy = offsetStart; iy <= offsetEnd; ++iy)
                {
                    for (int ix = offsetStart; ix <= offsetEnd; ++ix)
                    {

                    }
                }
            }
        }
    }
}

void GenerateBN_Void_Cluster(
    std::vector<uint8_t>& blueNoise,
    size_t width
)
{
    std::vector<bool> binaryPattern;
    MakeInitialBinaryPattern(binaryPattern, width);


    //std::vector<size_t> ranks(width*width, 0);
}