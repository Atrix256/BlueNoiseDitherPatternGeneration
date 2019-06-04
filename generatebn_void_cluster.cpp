#define _CRT_SECURE_NO_WARNINGS

#include "generatebn_void_cluster.h"
#include "whitenoise.h"

// TODO: temp!
#include "stb/stb_image_write.h"

template <bool FindTightestCluster>
static void FindTightestClusterOrLargestVoid(const std::vector<bool>& binaryPattern, size_t width, int &bestPixelX, int& bestPixelY)
{
    static const float c_sigma = 1.5f;
    static const float c_2sigmaSquared = 2.0f * c_sigma * c_sigma;

    int offsetStart = -int(width) / 2;
    int offsetEnd = int(width) / 2;
    if ((width & 1) == 0)
        offsetEnd--;

    // find the pixel with the best score
    float bestPixelValue = FindTightestCluster ? -FLT_MAX : FLT_MAX;
    bestPixelX = -1;
    bestPixelY = -1;

    for (int basey = 0; basey < width; ++basey)
    {
        for (int basex = 0; basex < width; ++basex)
        {
            // only consider 1s or 0s as appropriate
            if (binaryPattern[basey*width + basex] != FindTightestCluster)
                continue;

            // calculate an energy type metric to find voids and clusters
            float energy = 0.0f;
            for (int iy = offsetStart; iy <= offsetEnd; ++iy)
            {
                int y = (basey + iy + int(width)) % int(width);
                for (int ix = offsetStart; ix <= offsetEnd; ++ix)
                {
                    int x = (basex + ix + int(width)) % int(width);

                    // only 1's give energy
                    if (!binaryPattern[y*width + x])
                        continue;

                    float distanceSquared = float(ix*ix) + float(iy*iy);
                    energy += exp(-distanceSquared / c_2sigmaSquared);
                }
            }

            // keep track of which pixel had the highest energy
            if ((FindTightestCluster && energy > bestPixelValue) || (!FindTightestCluster && energy < bestPixelValue))
            {
                bestPixelValue = energy;
                bestPixelX = basex;
                bestPixelY = basey;
            }
        }
    }
}

void MakeInitialBinaryPattern(std::vector<bool>& binaryPattern, size_t width)
{

    std::mt19937 rng(GetRNGSeed());
    std::uniform_int_distribution<size_t> dist(0, width*width);

    binaryPattern.resize(width*width, false);
    size_t ones = width * width / 2;
    for (size_t index = 0; index < width; ++index)
    {
        size_t pixel = dist(rng);
        binaryPattern[pixel] = true;
    }



    // TODO: multithread
    // TODO: limit to a 3 sigma search area for the inner loops

    int iterationCount = 0;
    while (1)
    {
        iterationCount++;

        // find the location of the tightest cluster
        int tightestClusterX = -1;
        int tightestClusterY = -1;
        FindTightestClusterOrLargestVoid<true>(binaryPattern, width, tightestClusterX, tightestClusterY);

        // TODO: temp
        int trueCountStart = 0;
        for (bool b : binaryPattern)
        {
            if (b)
                trueCountStart++;
        }

        // remove the 1 from the tightest cluster
        binaryPattern[tightestClusterY*width + tightestClusterX] = false;

        // TODO: temp
        int trueCountA = 0;
        for (bool b : binaryPattern)
        {
            if (b)
                trueCountA++;
        }

        // TODO: temp
        if (iterationCount == 15)
        {
            int ijkl = 0;
        }

        // find the largest void
        int largestVoidX = -1;
        int largestVoidY = -1;
        FindTightestClusterOrLargestVoid<false>(binaryPattern, width, largestVoidX, largestVoidY);

        // TODO: temp
        int trueCountB = 0;
        for (bool b : binaryPattern)
        {
            if (b)
                trueCountB++;
        }

        // put the 1 in the largest void
        binaryPattern[largestVoidY*width + largestVoidX] = true;

        // TODO: temp
        int trueCountC = 0;
        for (bool b : binaryPattern)
        {
            if (b)
                trueCountC++;
        }

        // TODO: temp!  Keep it around though. remove = red, add = green. when done, the pixel will be yellow!
        // TODO: make a #define to spit this out or not. spit out images named by loop number
        std::vector<uint8_t> binaryPatternImage(width*width*3);
        for (size_t index = 0; index < width*width; ++index)
        {
            bool isCluster = (index == tightestClusterY * width + tightestClusterX);
            bool isVoid = (index == largestVoidY * width + largestVoidX);

            if (isCluster == isVoid)
            {
                if (isCluster)
                {
                    binaryPatternImage[index * 3 + 0] = 255;
                    binaryPatternImage[index * 3 + 1] = 255;
                    binaryPatternImage[index * 3 + 2] = 0;
                }
                else
                {
                    binaryPatternImage[index * 3 + 0] = binaryPattern[index] ? 255 : 0;
                    binaryPatternImage[index * 3 + 1] = binaryPattern[index] ? 255 : 0;
                    binaryPatternImage[index * 3 + 2] = binaryPattern[index] ? 255 : 0;
                }
            }
            else if (isCluster)
            {
                binaryPatternImage[index * 3 + 0] = 255;
                binaryPatternImage[index * 3 + 1] = 0;
                binaryPatternImage[index * 3 + 2] = 0;
            }
            else if (isVoid)
            {
                binaryPatternImage[index * 3 + 0] = 0;
                binaryPatternImage[index * 3 + 1] = 255;
                binaryPatternImage[index * 3 + 2] = 0;
            }
        }

        char fileName[256];
        sprintf(fileName, "out/vcbp%i.png", iterationCount);
        stbi_write_png(fileName, int(width), int(width), 3, binaryPatternImage.data(), 0);

        // TODO: temp
        int trueCountEnd = 0;
        for (bool b : binaryPattern)
        {
            if (b)
                trueCountEnd++;
        }

        printf("true count: %i to %i\n", trueCountStart, trueCountEnd);

        // exit condition. the pattern is stable
        if (tightestClusterX == largestVoidX && tightestClusterY == largestVoidY)
            break;
    }

    int ijkl = 0;
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