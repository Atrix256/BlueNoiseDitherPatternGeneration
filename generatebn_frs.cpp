#include "generatebn_frs.h"
#include "whitenoise.h"
#include "scoped_timer.h"

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

    // initialize data
    std::vector<bool> binaryPattern(width*width, false);
    std::vector<size_t> ranks(width*width, ~size_t(0));
    std::vector<double> LUT(width*width, 0.0);

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
        double bestCandidateValue = makeBlueNoise ? DBL_MAX : -DBL_MAX;

        for (size_t candidateIndex = 0, candidateCount = std::min<size_t>(emptyPixels.size() / 2, 1); candidateIndex <= candidateCount; ++candidateIndex)
        {
            size_t pixel = emptyPixels[candidateIndex];
            double pixelValue = LUT[pixel];

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

        // take the winning pixel
        binaryPattern[winningPixel] = true;
        ranks[winningPixel] = insertPointIndex;
        emptyPixels.erase(emptyPixels.begin() + winningCandidateIndex);
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
// TODO: V&C might benefit from using omp to write to the LUT. try it!

// Note: optimized by using a LUT, like void and cluster. also uses OMP to write to the LUT multithreadedly.
// NOTE: i tried getting rid of the shuffle by generating 1 bit per item that said whether it should be taken or not.
// - This made very structured results which confused me. AT first i thought it was that the order wasn't randomized, but since it takes the best, that shouldn't matter.
// - It also was slower, so i backed off.
