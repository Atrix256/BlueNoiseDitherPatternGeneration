#define _CRT_SECURE_NO_WARNINGS

#include "generatebn_void_cluster.h"
#include "whitenoise.h"
#include "convert.h"

// TODO: temp!
#include "stb/stb_image_write.h"

// TODO: temp!
#include <unordered_set>

template <bool FindTightestCluster, bool ReverseMinorityMajority>
static void FindTightestClusterOrLargestVoid(const std::vector<bool>& binaryPattern, size_t width, int &bestPixelX, int& bestPixelY)
{
    static const float c_sigma = 1.5f;
    static const float c_2sigmaSquared = 2.0f * c_sigma * c_sigma;

    static const bool c_minorityPixel = ReverseMinorityMajority ? false : true;
    static const bool c_majorityPixel = ReverseMinorityMajority ? true : false;

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
            // Finding the tightest cluster means only considering minority pixels
            // Finding the largest void means only considering majority pixels
            bool isMinorityPixel = binaryPattern[basey*width + basex] == c_minorityPixel;
            if (isMinorityPixel != FindTightestCluster)
                continue;

            // calculate an energy type metric to find voids and clusters
            float energy = 0.0f;
            for (int iy = offsetStart; iy <= offsetEnd; ++iy)
            {
                int y = (basey + iy + int(width)) % int(width);
                for (int ix = offsetStart; ix <= offsetEnd; ++ix)
                {
                    int x = (basex + ix + int(width)) % int(width);

                    // only minority pixels give energy
                    if (binaryPattern[y*width + x] != c_minorityPixel)
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
        FindTightestClusterOrLargestVoid<true, false>(binaryPattern, width, tightestClusterX, tightestClusterY);

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
        FindTightestClusterOrLargestVoid<false, false>(binaryPattern, width, largestVoidX, largestVoidY);

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

        // TODO: make larger with nearest neighbor, so people can see it
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

// Phase 1: Start with initial binary pattern and remove the tightest cluster until there are none left, entering ranks for those pixels
void Phase1(std::vector<bool>& binaryPattern, std::vector<size_t>& ranks, size_t width)
{
    // count how many ones there are
    size_t ones = 0;
    for (bool b : binaryPattern)
    {
        if (b)
            ones++;
    }

    // remove the tightest cluster repeatedly
    while (ones > 0)
    {
        int bestX, bestY;
        FindTightestClusterOrLargestVoid<true, false>(binaryPattern, width, bestX, bestY);
        binaryPattern[bestY * width + bestX] = false;
        ones--;
        ranks[bestY*width + bestX] = ones;
    }
}

// Phase 2: Start with initial binary pattern and add points to the largest void until half the pixels are white, entering ranks for those pixels
void Phase2(std::vector<bool>& binaryPattern, std::vector<size_t>& ranks, size_t width)
{
    // count how many ones there are
    size_t ones = 0;
    for (bool b : binaryPattern)
    {
        if (b)
            ones++;
    }

    // add to the largest void repeatedly
    while (ones <= (width*width/2))
    {
        int bestX, bestY;
        FindTightestClusterOrLargestVoid<false, false>(binaryPattern, width, bestX, bestY);
        binaryPattern[bestY * width + bestX] = true;
        ranks[bestY*width + bestX] = ones;
        ones++;
    }
}

// Phase 3: Continue with the last binary pattern, repeatedly find the tightest cluster of 0s and insert a 1 into them
void Phase3(std::vector<bool>& binaryPattern, std::vector<size_t>& ranks, size_t width)
{
    // count how many ones there are
    size_t ones = 0;
    for (bool b : binaryPattern)
    {
        if (b)
            ones++;
    }

    // add to the largest void of 1's repeatedly
    while (1)
    {
        int bestX, bestY;
        FindTightestClusterOrLargestVoid<true, true>(binaryPattern, width, bestX, bestY);

        // exit condition. all done!
        if (bestX < 0 || bestY < 0)
            break;

        binaryPattern[bestY * width + bestX] = true;
        ranks[bestY*width + bestX] = ones;
        ones++;
    }
}

void GenerateBN_Void_Cluster(std::vector<uint8_t>& blueNoise, size_t width)
{
    // make the initial binary pattern
    std::vector<bool> initialBinaryPattern;
    MakeInitialBinaryPattern(initialBinaryPattern, width);

    // Phase 1: Start with initial binary pattern and remove the tightest cluster until there are none left, entering ranks for those pixels
    std::vector<size_t> ranks(width*width, ~size_t(0));
    std::vector<bool> binaryPattern = initialBinaryPattern;
    Phase1(binaryPattern, ranks, width);

    // Phase 2: Start with initial binary pattern and add points to the largest void until half the pixels are white, entering ranks for those pixels
    binaryPattern = initialBinaryPattern;
    Phase2(binaryPattern, ranks, width);

    // Phase 3: Continue with the last binary pattern, repeatedly find the tightest cluster of 0s and insert a 1 into them
    Phase3(binaryPattern, ranks, width);

    // convert to U8
    blueNoise.resize(width*width);
    double deltaQ = double(width*width-1) / 255.0;
    double multiplier = deltaQ / double(width*width);
    for (size_t index = 0; index < width*width; ++index)
    {
        float percent = float(ranks[index]) / float(width*width - 1);
        blueNoise[index] = FromFloat<uint8_t>(percent);
    }
}
