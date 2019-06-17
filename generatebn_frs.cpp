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
    size_t width,
    bool makeBlueNoise
)
{
    std::mt19937 rng(GetRNGSeed());
    static_assert(sizeof(unsigned int) == 4, "This code assumes sizeof(unsigned int) is 4, aka that rng() returns a uint32");

    // initialize data
    std::vector<bool> binaryPattern(width*width, false);
    std::vector<size_t> ranks(width*width, ~size_t(0));
    std::vector<double> LUT(width*width, 0.0);

    // put a first point in
    {
        std::uniform_int_distribution<size_t> dist(0, width*width - 1);
        size_t firstPoint = dist(rng);

        binaryPattern[firstPoint] = true;
        ranks[firstPoint] = 0;
        WriteLutValue(LUT, width, firstPoint % width, firstPoint / width);
    }

    // put all of the rest of the points in
    std::vector<uint32_t> randomUINTs;
    for (size_t insertPointIndex = 1; insertPointIndex < width*width; ++insertPointIndex)
    {
        // find the lowest score when looking at roughly half of the empty pixels
        std::vector<size_t> bestCandidateIndices;
        double bestCandidateValue = makeBlueNoise ? DBL_MAX : -DBL_MAX;

        // generate random numbers for use to know which empty pixels to test or not
        size_t randomBitsNeeded = width*width - insertPointIndex;
        size_t randomUINTsNeeded = randomBitsNeeded / 32;
        if ((randomBitsNeeded % 32) > 0)
            randomUINTsNeeded++;
        randomUINTs.resize(randomUINTsNeeded);
        for (uint32_t& v : randomUINTs)
            v = rng();
        uint32_t rndMask = 1;
        uint32_t rndIndex = 0;

        // force a random candidate to be set to 1, so there is always at least one considered.
        if (randomBitsNeeded == 1)
        {
            randomUINTs[0] |= 1;
        }
        else
        {
            std::uniform_int_distribution<size_t> dist(0, randomBitsNeeded - 1);
            size_t chosenBit = dist(rng);
            size_t chosenIndex = chosenBit / 32;
            uint32_t chosenMask = 1 << (chosenBit % 32);
            randomUINTs[chosenIndex] |= chosenMask;
        }

        // find the best candidate.
        for (size_t candidateIndex = 0; candidateIndex < width*width; ++candidateIndex)
        {
            // if this pixel is already set, skip it
            if (binaryPattern[candidateIndex])
                continue;

            // use a random bit to see if this pixel should be considered or not.
            bool passedTest = randomUINTs[rndIndex] & rndMask;
            rndMask *= 2;
            if (rndMask == 0)
            {
                rndMask = 1;
                rndIndex++;
            }
            if (!passedTest)
            {
                continue;
            }

            // consider this pixel as a candidate
            double pixelValue = LUT[candidateIndex];
            if (pixelValue == bestCandidateValue)
            {
                bestCandidateIndices.push_back(candidateIndex);
            }
            else if ((makeBlueNoise && pixelValue < bestCandidateValue) || (!makeBlueNoise && pixelValue > bestCandidateValue))
            {
                bestCandidateIndices.clear();
                bestCandidateValue = pixelValue;
                bestCandidateIndices.push_back(candidateIndex);
            }
        }

        // if there are multiple winners, randomly select the right one
        size_t winningPixel = ~size_t(0);
        if (bestCandidateIndices.size() == 1)
        {
            winningPixel = bestCandidateIndices[0];
        }
        else
        {
            std::uniform_int_distribution<size_t> dist(0, bestCandidateIndices.size() - 1);
            winningPixel = bestCandidateIndices[dist(rng)];
        }

        // take the winning pixel
        binaryPattern[winningPixel] = true;
        ranks[winningPixel] = insertPointIndex;
        WriteLutValue(LUT, width, winningPixel % width, winningPixel / width);

        // show what percentage we are done
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

// TODO: something broke when you removed the shuffle. try going back to it?
// TODO: can this make red noise? try it!
// TODO: convert all other things to use omp instead of std::thread!
// TODO: V&C might benefit from using omp to write to the LUT. try it!

// Note: optimized by using a LUT, like void and cluster. also uses OMP to write to the LUT multithreadedly.
// NOTE: i tried getting rid of the shuffle by generating 1 bit per item that said whether it should be taken or not.
// - This made very structured results which confused me.
// - I realized these aren't the same, because a shuffle randomizes the order. doh!
// actually... order shouldn't matter since i'm going through all of them anyways, and randomly choosing from lowest values. WTF?!
