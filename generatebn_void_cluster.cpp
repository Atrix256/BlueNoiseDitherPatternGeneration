#define _CRT_SECURE_NO_WARNINGS

#include "generatebn_void_cluster.h"
#include "whitenoise.h"
#include "convert.h"
#include "stb/stb_image_write.h"
#include "scoped_timer.h"

#include <thread>
#include <atomic>

static const float c_sigma = 1.5f;
static const float c_2sigmaSquared = 2.0f * c_sigma * c_sigma;
static const int c_3sigmaint = int(ceil(c_sigma * 3.0f));

// TODO: temp?
static void SaveLUTImage(const std::vector<bool>& binaryPattern, std::vector<float>& LUT, size_t width, const char* fileName)
{
    // get the LUT min and max
    float LUTMin = LUT[0];
    float LUTMax = LUT[0];
    for (float f : LUT)
    {
        LUTMin = std::min(LUTMin, f);
        LUTMax = std::max(LUTMax, f);
    }

    size_t c_scale = 4;

    std::vector<uint8_t> image(width*width * c_scale*c_scale * 3);
    for (size_t index = 0; index < width*width*c_scale*c_scale; ++index)
    {
        size_t x = (index % (width * c_scale)) / c_scale;
        size_t y = index / (width * c_scale * c_scale);

        float percent = (LUT[y*width + x] - LUTMin) / (LUTMax - LUTMin);
        uint8_t value = FromFloat<uint8_t>(percent);

        image[index * 3 + 0] = value;
        image[index * 3 + 1] = value;
        image[index * 3 + 2] = value;

        if (binaryPattern[y*width + x])
        {
            image[index * 3 + 0] = 0;
            image[index * 3 + 1] = 255;
            image[index * 3 + 2] = 0;
        }
    }
    stbi_write_png(fileName, int(width*c_scale), int(width*c_scale), 3, image.data(), 0);
}

static bool FindTightestClusterLUT(const std::vector<float>& LUT, const std::vector<bool>& binaryPattern, size_t width, int &bestPixelX, int& bestPixelY)
{
    float bestValue = -FLT_MAX;
    size_t bestIndex = ~size_t(0);
    for (size_t index = 1, count = LUT.size(); index < count; ++index)
    {
        if (binaryPattern[index] && LUT[index] > bestValue)
        {
            bestValue = LUT[index];
            bestIndex = index;
        }
    }

    if (bestIndex == ~size_t(0))
        return false;

    bestPixelX = int(bestIndex % width);
    bestPixelY = int(bestIndex / width);

    return true;
}

static bool FindLargestVoidLUT(const std::vector<float>& LUT, const std::vector<bool>& binaryPattern, size_t width, int &bestPixelX, int& bestPixelY)
{
    float bestValue = FLT_MAX;
    size_t bestIndex = ~size_t(0);
    for (size_t index = 1, count = LUT.size(); index < count; ++index)
    {
        if (!binaryPattern[index] && LUT[index] < bestValue)
        {
            bestValue = LUT[index];
            bestIndex = index;
        }
    }

    if (bestIndex == ~size_t(0))
        return false;

    bestPixelX = int(bestIndex % width);
    bestPixelY = int(bestIndex / width);

    return true;
}

static void WriteLUTValue(std::vector<float>& LUT, size_t width, bool value, int basex, int basey)
{
    // TODO: we should probably do +/- 3 sigma always but clamp to half width half height

    int offsetStart = -int(width) / 2;
    int offsetEnd = int(width) / 2;
    if ((width & 1) == 0)
        offsetEnd--;

    // 99.7% of influence is within 3 sigma
    offsetStart = std::max(offsetStart, -c_3sigmaint);
    offsetEnd = std::min(offsetEnd, c_3sigmaint);

    // add or subtract the energy from this pixel
    for (int offsetY = offsetStart; offsetY <= offsetEnd; ++offsetY)
    {
        int y = (basey + offsetY + int(width)) % int(width);

        for (int offsetX = offsetStart; offsetX <= offsetEnd; ++offsetX)
        {
            int x = (basex + offsetX + int(width)) % int(width);

            float distanceSquared = float(offsetX*offsetX) + float(offsetY*offsetY);
            float energy = exp(-distanceSquared / c_2sigmaSquared) * (value ? 1.0f : -1.0f);
            LUT[y*width + x] += energy;
        }
    }
}

static void MakeLUT(const std::vector<bool>& binaryPattern, std::vector<float>& LUT, size_t width, bool writeOnes)
{
    LUT.clear();
    LUT.resize(width*width, 0.0f);
    for (size_t index = 0; index < width*width; ++index)
    {
        if (binaryPattern[index] == writeOnes)
        {
            int x = int(index % width);
            int y = int(index / width);
            WriteLUTValue(LUT, width, writeOnes, x, y);
        }
    }
}

template <bool FindTightestCluster, bool ReverseMinorityMajority>
static void FindTightestClusterOrLargestVoid(const std::vector<bool>& binaryPattern, size_t width, int &bestPixelX, int& bestPixelY)
{
    static const bool c_minorityPixel = ReverseMinorityMajority ? false : true;
    static const bool c_majorityPixel = ReverseMinorityMajority ? true : false;

    // TODO: we should probably do +/- 3 sigma always but clamp to half width half height

    int offsetStart = -int(width) / 2;
    int offsetEnd = int(width) / 2;
    if ((width & 1) == 0)
        offsetEnd--;

    // 99.7% of influence is within 3 sigma
    offsetStart = std::max(offsetStart, -c_3sigmaint);
    offsetEnd = std::min(offsetEnd, c_3sigmaint);

    std::vector<std::thread> threads;
    threads.resize(std::thread::hardware_concurrency());
    std::atomic<size_t> atomicrow(0);
    std::vector<float> energies(width, 0.0f);

    // find the pixel with the best score in each row
    bestPixelX = -1;
    bestPixelY = -1;
    struct BestPixel
    {
        float value;
        int x, y;
    };
    std::vector<BestPixel> rowBestPixels(width, { FindTightestCluster ? -FLT_MAX : FLT_MAX, -1, -1 });

    // split the work among however many threads are available
    for (size_t threadIndex = 0; threadIndex < threads.size(); ++threadIndex)
    {
        threads[threadIndex] = std::thread(
            [width, offsetStart, offsetEnd, &atomicrow, &binaryPattern, &rowBestPixels]()
            {
                // get first row to process
                size_t row = atomicrow.fetch_add(1);

                // process rows until we run out
                while (row < width)
                {
                    int basey = int(row);

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
                        if ((FindTightestCluster && energy > rowBestPixels[row].value) || (!FindTightestCluster && energy < rowBestPixels[row].value))
                        {
                            rowBestPixels[row].value = energy;
                            rowBestPixels[row].x = basex;
                            rowBestPixels[row].y = basey;
                        }
                    }

                    // get next row to process
                    row = atomicrow.fetch_add(1);
                }
            }
        );
    }

    // wait for all threads to be done
    for (std::thread& t : threads)
        t.join();

    // get the best from all rows
    float bestValue = rowBestPixels[0].value;
    bestPixelX = rowBestPixels[0].x;
    bestPixelY = rowBestPixels[0].y;
    for (size_t i = 1; i < rowBestPixels.size(); ++i)
    {
        if ((FindTightestCluster && rowBestPixels[i].value > bestValue) || (!FindTightestCluster && rowBestPixels[i].value < bestValue))
        {
            bestValue = rowBestPixels[i].value;
            bestPixelX = rowBestPixels[i].x;
            bestPixelY = rowBestPixels[i].y;
        }
    }
}

#if true//SAVE_VOIDCLUSTER_INITIALBP()

static void SaveBinaryPattern(const std::vector<bool>& binaryPattern, size_t width, const char* baseFileName, int iterationCount, int tightestClusterX, int tightestClusterY, int largestVoidX, int largestVoidY)
{
    size_t c_scale = 4;

    std::vector<uint8_t> binaryPatternImage(width*width * c_scale*c_scale * 3);
    for (size_t index = 0; index < width*width*c_scale*c_scale; ++index)
    {
        size_t x = (index % (width * c_scale)) / c_scale;
        size_t y = index / (width * c_scale * c_scale);

        bool isCluster = (x == tightestClusterX && y == tightestClusterY);
        bool isVoid = (x == largestVoidX && y == largestVoidY);

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
                binaryPatternImage[index * 3 + 0] = binaryPattern[y*width+x] ? 255 : 0;
                binaryPatternImage[index * 3 + 1] = binaryPattern[y*width + x] ? 255 : 0;
                binaryPatternImage[index * 3 + 2] = binaryPattern[y*width + x] ? 255 : 0;
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
    sprintf(fileName, "%s%i.png", baseFileName, iterationCount);
    stbi_write_png(fileName, int(width*c_scale), int(width*c_scale), 3, binaryPatternImage.data(), 0);
}

#endif

static void MakeInitialBinaryPattern(std::vector<bool>& binaryPattern, size_t width, const char* baseFileName)
{
    ScopedTimer timer("Initial Pattern", false);

    std::mt19937 rng(GetRNGSeed());
    std::uniform_int_distribution<size_t> dist(0, width*width);

    std::vector<float> LUT;
    LUT.resize(width*width, 0.0f);

    binaryPattern.resize(width*width, false);
    size_t ones = width*width/16;
    for (size_t index = 0; index < ones; ++index)
    {
        size_t pixel = dist(rng);
        binaryPattern[pixel] = true;
        WriteLUTValue(LUT, width, true, int(pixel % width), int(pixel / width));
    }

    SaveLUTImage(binaryPattern, LUT, width, "out/_init.png");

    int iterationCount = 0;
    while (1)
    {
        printf("\r%i iterations", iterationCount);
        iterationCount++;

        // find the location of the tightest cluster
        int tightestClusterX = -1;
        int tightestClusterY = -1;
        //FindTightestClusterOrLargestVoid<true, false>(binaryPattern, width, tightestClusterX, tightestClusterY);
        FindTightestClusterLUT(LUT, binaryPattern, width, tightestClusterX, tightestClusterY);

        // remove the 1 from the tightest cluster
        binaryPattern[tightestClusterY*width + tightestClusterX] = false;
        WriteLUTValue(LUT, width, false, tightestClusterX, tightestClusterY);

        // find the largest void
        int largestVoidX = -1;
        int largestVoidY = -1;
        //FindTightestClusterOrLargestVoid<false, false>(binaryPattern, width, largestVoidX, largestVoidY);
        FindLargestVoidLUT(LUT, binaryPattern, width, largestVoidX, largestVoidY);

        // put the 1 in the largest void
        binaryPattern[largestVoidY*width + largestVoidX] = true;
        WriteLUTValue(LUT, width, true, largestVoidX, largestVoidY);

        #if SAVE_VOIDCLUSTER_INITIALBP()
        // save the binary pattern out for debug purposes
        SaveBinaryPattern(binaryPattern, width, baseFileName, iterationCount, tightestClusterX, tightestClusterY, largestVoidX, largestVoidY);
        #endif

        // exit condition. the pattern is stable
        if (tightestClusterX == largestVoidX && tightestClusterY == largestVoidY)
            break;
    }
    printf("\n");

    SaveBinaryPattern(binaryPattern, width, "out/_LUT_YES", 0, -1, -1, -1, -1);
}

// Phase 1: Start with initial binary pattern and remove the tightest cluster until there are none left, entering ranks for those pixels
static void Phase1(std::vector<bool>& binaryPattern, std::vector<float>& LUT, std::vector<size_t>& ranks, size_t width)
{
    ScopedTimer timer("Phase 1", false);

    // count how many ones there are
    size_t ones = 0;
    for (bool b : binaryPattern)
    {
        if (b)
            ones++;
    }
    size_t startingOnes = ones;

    // remove the tightest cluster repeatedly
    while (ones > 0)
    {
        printf("\r%i%%", int(100.0f * (1.0f - float(ones) / float(startingOnes))));

        int bestX, bestY;
        FindTightestClusterLUT(LUT, binaryPattern, width, bestX, bestY);
        binaryPattern[bestY * width + bestX] = false;
        WriteLUTValue(LUT, width, false, bestX, bestY);
        ones--;
        ranks[bestY*width + bestX] = ones;
    }
    printf("\n");
}

struct Point
{
    size_t x;
    size_t y;
};
typedef std::vector<Point> TPoints;
typedef std::vector<TPoints> TPointGrid;

static bool DistanceSqToClosestPoint(const TPoints& points, const Point& point, float& minDistSq)
{
    if (points.size() == 0)
        return false;

    // calculate the closest distance from this point to an existing sample
    for (const Point& p : points)
    {
        float distx = float(p.x) - float(point.x);
        float disty = float(p.y) - float(point.y);
        float distSq = distx * distx + disty * disty;
        if (distSq < minDistSq)
            minDistSq = distSq;
    }
    return true;
}

static float DistanceSqToClosestPoint(const TPointGrid& grid, size_t cellCount, size_t cellSize, const Point& point)
{
    const int basex = int(point.x / cellSize);
    const int basey = int(point.y / cellSize);

    const int maxRadius = int(cellCount / 2);

    float minDistSq = FLT_MAX;
    bool foundAPoint = false;
    bool didAnExtraRing = false;

    for (int radius = 0; radius <= maxRadius; ++radius)
    {
        // top and bottom rows
        {
            for (int offsetX = -radius; offsetX <= radius; ++offsetX)
            {
                int x = int(basex + offsetX + cellCount) % int(cellCount);

                int offsetY = -radius;
                int y = int(basey + offsetY + cellCount) % int(cellCount);
                foundAPoint |= DistanceSqToClosestPoint(grid[y*cellCount + x], point, minDistSq);

                offsetY = radius;
                y = int(basey + offsetY + cellCount) % int(cellCount);
                foundAPoint |= DistanceSqToClosestPoint(grid[y*cellCount + x], point, minDistSq);
            }
        }

        // left and right
        {
            for (int offsetY = -radius + 1; offsetY <= radius - 1; ++offsetY)
            {
                int y = int(basey + offsetY + cellCount) % int(cellCount);

                int offsetX = -radius;
                int x = int(basex + offsetX + cellCount) % int(cellCount);
                foundAPoint |= DistanceSqToClosestPoint(grid[y*cellCount + x], point, minDistSq);

                offsetX = +radius;
                x = int(basex + offsetX + cellCount) % int(cellCount);
                foundAPoint |= DistanceSqToClosestPoint(grid[y*cellCount + x], point, minDistSq);
            }
        }

        // we stop when we've found a point, then do another ring to make sure there isn't something closer to what we found.
        if (foundAPoint)
        {
            if (didAnExtraRing)
                break;
            else
                didAnExtraRing = true;
        }
    }

    return minDistSq;
}

static void AddPointToPointGrid(TPointGrid& grid, size_t cellCount, size_t cellSize, const Point& point)
{
    Point cell;
    cell.x = point.x / cellSize;
    cell.y = point.y / cellSize;
    grid[cell.y * cellCount + cell.x].push_back(point);
}

// This replaces "Initial Binary Pattern" and "Phase 1" in the void and cluster algorithm.
// Initial binary pattern makes blue noise distributed points.
// Phase 1 makes them be progressive, so any points from 0 to N are blue noise.
// Mitchell's best candidate algorithm makes progressive blue noise faster than those two steps do it.
// https://blog.demofox.org/2017/10/20/generating-blue-noise-sample-points-with-mitchells-best-candidate-algorithm/
static void MitchellsBestCandidate(std::vector<bool>& binaryPattern, std::vector<size_t>& ranks, size_t width)
{
    ScopedTimer timer("Mitchells Best Candidate", false);

    std::mt19937 rng(GetRNGSeed());
    std::uniform_int_distribution<size_t> dist(0, width*width);

    binaryPattern.resize(width*width, false);
    ranks.resize(width*width, ~size_t(0));

    static const size_t gridCellCount = 32;
    TPointGrid grid(gridCellCount*gridCellCount);
    const size_t gridCellSize = width / gridCellCount;

    //std::vector<Point> points;

    // TODO: V&C timings had this at width*width/4.  It seems better to have it here at /16. need to compare timing vs V&C though for same amount.

    size_t ones = width * width / 16;
    for (size_t i = 0; i < ones; ++i)
    {
        printf("\r%i%%", int(100.0f * float(i) / float(ones - 1)));

        // we scale up the candidates each iteration like in the paper, to keep frequency behavior consistent
        size_t numCandidates = i + 1;

        // keep the candidate that is farthest from the closest existing point
        float bestDistanceSq = 0.0f;
        Point best;
        for (size_t candidate = 0; candidate < numCandidates; ++candidate)
        {
            size_t index = dist(rng);
            Point c;
            c.x = index % width;
            c.y = index / width;

            float minDistSq = DistanceSqToClosestPoint(grid, gridCellCount, gridCellSize, c);

            /*
            // calculate the closest distance from this point to an existing sample
            float minDistSq = FLT_MAX;
            for (const Point& p : points)
            {
                float distx = float(p.x - c.x);
                float disty = float(p.y - c.y);
                float distSq = distx * distx + disty * disty;
                if (distSq < minDistSq)
                    minDistSq = distSq;
            }
            */

            if (minDistSq > bestDistanceSq)
            {
                bestDistanceSq = minDistSq;
                best = c;
            }
        }

        // take the best candidate
        binaryPattern[best.y * width + best.x] = true;
        ranks[best.y * width + best.x] = i;
        //points.push_back(best);
        AddPointToPointGrid(grid, gridCellCount, gridCellSize, best);
    }
    printf("\n");
}

// Phase 2: Start with initial binary pattern and add points to the largest void until half the pixels are white, entering ranks for those pixels
static void Phase2(std::vector<bool>& binaryPattern, std::vector<float>& LUT, std::vector<size_t>& ranks, size_t width)
{
    ScopedTimer timer("Phase 2", false);

    // count how many ones there are
    size_t ones = 0;
    for (bool b : binaryPattern)
    {
        if (b)
            ones++;
    }
    size_t startingOnes = ones;
    size_t onesToDo = (width*width / 2) - startingOnes;

    // add to the largest void repeatedly
    while (ones <= (width*width/2))
    {
        size_t onesDone = ones - startingOnes;
        printf("\r%i%%", int(100.0f * float(onesDone) / float(onesToDo)));

        int bestX, bestY;
        FindLargestVoidLUT(LUT, binaryPattern, width, bestX, bestY);
        binaryPattern[bestY * width + bestX] = true;
        WriteLUTValue(LUT, width, true, bestX, bestY);
        ranks[bestY*width + bestX] = ones;
        ones++;
    }
    printf("\n");
}

// Phase 3: Continue with the last binary pattern, repeatedly find the tightest cluster of 0s and insert a 1 into them
static void Phase3(std::vector<bool>& binaryPattern, std::vector<float>& LUT, std::vector<size_t>& ranks, size_t width)
{
    ScopedTimer timer("Phase 3", false);

    // count how many ones there are
    size_t ones = 0;
    for (bool b : binaryPattern)
    {
        if (b)
            ones++;
    }
    size_t startingOnes = ones;
    size_t onesToDo = (width*width) - startingOnes;

    // add 1 to the largest cluster of 0's repeatedly
    int bestX, bestY;
    while (FindLargestVoidLUT(LUT, binaryPattern, width, bestX, bestY))
    {
        size_t onesDone = ones - startingOnes;
        printf("\r%i%%", int(100.0f * float(onesDone) / float(onesToDo)));

        WriteLUTValue(LUT, width, true, bestX, bestY);
        binaryPattern[bestY * width + bestX] = true;
        ranks[bestY*width + bestX] = ones;
        ones++;
    }
    printf("\n");
}

void GenerateBN_Void_Cluster(std::vector<uint8_t>& blueNoise, size_t width, const char* baseFileName)
{

#if 1
    // make the initial binary pattern
    std::vector<bool> initialBinaryPattern;
    MakeInitialBinaryPattern(initialBinaryPattern, width, baseFileName);

    // Phase 1: Start with initial binary pattern and remove the tightest cluster until there are none left, entering ranks for those pixels
    std::vector<size_t> ranks(width*width, ~size_t(0));
    std::vector<bool> binaryPattern = initialBinaryPattern;
    std::vector<float> LUT;
    MakeLUT(binaryPattern, LUT, width, true);
    Phase1(binaryPattern, LUT, ranks, width);

#else

    // replace initial binary pattern and phase 1 with Mitchell's best candidate algorithm
    std::vector<bool> initialBinaryPattern;
    std::vector<size_t> ranks;
    MitchellsBestCandidate(initialBinaryPattern, ranks, width);
    SaveBinaryPattern(initialBinaryPattern, width, "out/_blah", 0, -1, -1, -1, -1);

    // TODO: put these under phase 2 after you get rid of the other
    std::vector<bool> binaryPattern;
    std::vector<float> LUT;
#endif

    // Phase 2: Start with initial binary pattern and add points to the largest void until half the pixels are white, entering ranks for those pixels
    binaryPattern = initialBinaryPattern;
    MakeLUT(binaryPattern, LUT, width, true);
    Phase2(binaryPattern, LUT, ranks, width);

    // Phase 3: Continue with the last binary pattern, repeatedly find the tightest cluster of 0s and insert a 1 into them
    MakeLUT(binaryPattern, LUT, width, false);
    Phase3(binaryPattern, LUT, ranks, width);

    // convert to U8
    {
        ScopedTimer timer("Converting to U8", false);

        blueNoise.resize(width*width);
        double deltaQ = double(width*width - 1) / 255.0;
        double multiplier = deltaQ / double(width*width);
        for (size_t index = 0; index < width*width; ++index)
        {
            float percent = float(ranks[index]) / float(width*width - 1);
            blueNoise[index] = FromFloat<uint8_t>(percent);
        }
    }
}
// TODO: for initial binary pattern AND phase 1, maybe do mitchell's best candidate algorithm, adding to rank and binary pattern as you go? then go straight to phase 2.

// TODO: the histogram has 256's except for first and last value which are 255 and 257 respectively. Fix and check in results. Maybe wait to check in post profiling though.
// TODO: profile after doing 3 sigma limit
// TODO: boost up resolution after speeding things up
// TODO: show what phases it's doing and a percentage when possible
// TODO: round 3 sigma up.
// TODO: make a LUT instead of running the full scan each frame
// TODO: test it at the lowest and highest threshold levels to make sure it's g2g. Basically need to randomize ties. https://twitter.com/atrix256/status/1136391416395980800?s=12
