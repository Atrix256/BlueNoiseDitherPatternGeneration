#include "generatebn_frs.h"
#include "whitenoise.h"
#include "scoped_timer.h"
#include <thread>
#include <atomic>

static void WriteLutValue(std::vector<double>& LUT, size_t width, size_t locx, size_t locy)
{
    // process rows until we run out
    #pragma omp parallel for
    for (int y = 0; y < width; ++y)
    {
        // get y distance
        size_t disty = (y >= locy) ? (y - locy) : (locy - y);
        if (disty > width / 2)
            disty = width - disty;

        // process each column in this row
        for (size_t x = 0; x < width; ++x)
        {
            // get x distance
            size_t distx = (x >= locx) ? (x - locx) : (locx - x);
            if (distx > width / 2)
                distx = width - distx;

            // calculate real distance and energy, and add it into the table
            double distance = sqrt(double(distx*distx) + double(disty*disty));
            double energy = exp(-pow(distance / 1.5, 1.5));
            LUT[y*width + x] += energy;
        }
    }
}

void GenerateBN_FRS(
    std::vector<uint8_t>& blueNoise,
    size_t width
)
{
    std::mt19937 rng(GetRNGSeed());

    // initialize data
    std::vector<bool> binaryPattern(width*width, false);
    std::vector<size_t> ranks(width*width, ~size_t(0));
    std::vector<double> LUT(width*width, false);

    std::vector<size_t> emptyPixels;
    emptyPixels.resize(width*width);
    for (size_t i = 0; i < width*width; ++i)
        emptyPixels[i] = i;

    // put a first point in
    {
        std::uniform_int_distribution<size_t> dist(0, width*width - 1);
        size_t firstPoint = dist(rng);

        binaryPattern[firstPoint] = true;
        ranks[firstPoint] = 0;
        emptyPixels.erase(emptyPixels.begin() + firstPoint);
        WriteLutValue(LUT, width, firstPoint % width, firstPoint / width);
    }

    // put all of the rest of the points in
    for (size_t insertPointIndex = 1; insertPointIndex < width*width; ++insertPointIndex)
    {
        // shuffle the empty pixels
        std::shuffle(emptyPixels.begin(), emptyPixels.end(), rng);

        // find the lowest score in the first half of the empty pixels
        std::vector<size_t> bestCandidateIndices;
        double bestCandidateValue = DBL_MAX;
        for (size_t candidateIndex = 0, candidateCount = std::min<size_t>(emptyPixels.size() / 2, 1); candidateIndex <= candidateCount; ++candidateIndex)
        {
            size_t pixel = emptyPixels[candidateIndex];
            double pixelValue = LUT[pixel];

            if (pixelValue == bestCandidateValue)
            {
                bestCandidateIndices.push_back(candidateIndex);
            }
            else if (pixelValue < bestCandidateValue)
            {
                bestCandidateIndices.clear();
                bestCandidateValue = pixelValue;
                bestCandidateIndices.push_back(candidateIndex);
            }
        }

        size_t winningCandidateIndex = ~size_t(0);
        if (bestCandidateIndices.size() == 1)
        {
            winningCandidateIndex = bestCandidateIndices[0];
        }
        else
        {
            std::uniform_int_distribution<size_t> dist(0, bestCandidateIndices.size() - 1);
            winningCandidateIndex = bestCandidateIndices[dist(rng)];
        }

        size_t winningPixel = emptyPixels[winningCandidateIndex];

        binaryPattern[winningPixel] = true;
        ranks[winningPixel] = insertPointIndex;
        emptyPixels.erase(emptyPixels.begin() + winningCandidateIndex);
        WriteLutValue(LUT, width, winningPixel % width, winningPixel / width);

        printf("\r%i%%", int(100.0f * float(insertPointIndex) / float(width*width)));
    }

    // convert ranks to U8
    {
        blueNoise.resize(width*width);
        for (size_t index = 0; index < width*width; ++index)
            blueNoise[index] = uint8_t(ranks[index] * 256 / (width*width));
    }

    printf("\n");
}

// TODO: can we make shuffling faster somehow? we just want to test half of the points... maybe use rng to say if we should test one or not and never actually shuffle? probably faster. Can we get N random bits where N is the number of empty slots left?
// TODO: profile.
// TODO: can this make red noise? try it!

// Note: optimized by using a LUT
