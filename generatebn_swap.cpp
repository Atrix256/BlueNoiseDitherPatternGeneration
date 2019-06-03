#include "convert.h"
#include "generatebn_swap.h"
#include "whitenoise.h"

#include <thread>
#include <atomic>

inline float ToroidalDistanceSquared(float x1, float y1, float x2, float y2, float width)
{
    float dx = std::abs(x2 - x1);
    float dy = std::abs(y2 - y1);

    if (dx > 0.5f * width)
        dx = width - dx;

    if (dy > 0.5f * width)
        dy = width - dy;

    return (dx * dx + dy * dy);
}

template <bool LIMITTO3SIGMA>
float CalculateEnergy(const std::vector<float>& pixels, size_t width)
{
    static const float c_sigma_i = 2.1f;
    static const float c_sigma_s = 1.0f;

    static const int c_3Sigma_i = int(Clamp<size_t>(0, width / 2 - 1, size_t(ceil(c_sigma_i*3.0f))));

    size_t pixelCount = width * width;

    std::vector<std::thread> threads;
    threads.resize(std::thread::hardware_concurrency());
    std::atomic<size_t> atomicrow(0);
    std::vector<float> energies(width, 0.0f);

    // split the work among however many threads are available
    for (size_t threadIndex = 0; threadIndex < threads.size(); ++threadIndex)
    {
        threads[threadIndex] = std::thread(
            [pixelCount, width, &atomicrow, &pixels, &energies]()
            {
                // get first row to process
                size_t row = atomicrow.fetch_add(1);

                // process rows until we run out
                while (row < width)
                {
                    // for each pixel in this row...
                    for (size_t column = 0; column < width; ++column)
                    {
                        size_t p = row * width + column;

                        float px = float(p % width);
                        float py = float(p / width);
                        float pvalue = pixels[p];

                        // limit it to +/- 3 standard deviations which is 99.7% of the data
                        if (LIMITTO3SIGMA)
                        {
                            for (int oy = -c_3Sigma_i; oy <= c_3Sigma_i; ++oy)
                            {
                                float qy = py + float(oy);

                                int qyi = int(qy);
                                if (qyi < 0)
                                    qyi += int(width);
                                qyi = qyi % width;

                                for (int ox = -c_3Sigma_i; ox <= c_3Sigma_i; ++ox)
                                {
                                    float qx = px + float(ox);

                                    int qxi = int(qx);
                                    if (qxi < 0)
                                        qxi += int(width);
                                    qxi = qxi % width;

                                    float qvalue = pixels[qyi*width + qxi];

                                    float distanceSquared = ToroidalDistanceSquared(px, py, qx, qy, float(width));

                                    float leftTerm = (distanceSquared) / (c_sigma_i * c_sigma_i);

                                    float d = 1.0f;
                                    float rightTerm = (powf(std::abs(qvalue - pvalue), d / 2.0f)) / (c_sigma_s * c_sigma_s);

                                    float energy = exp(-leftTerm - rightTerm);

                                    energies[row] += energy;
                                    // TODO: when supporting multiple channel blue noise, right term needs treatment
                                }
                            }
                        }
                        else
                        {
                            for (size_t q = 0; q < pixelCount; ++q)
                            {
                                if (p == q)
                                    continue;

                                float qx = float(q % width);
                                float qy = float(q / width);
                                float qvalue = pixels[q];

                                float distanceSquared = ToroidalDistanceSquared(px, py, qx, qy, float(width));

                                float leftTerm = (distanceSquared) / (c_sigma_i * c_sigma_i);

                                float d = 1.0f;
                                float rightTerm = (powf(std::abs(qvalue - pvalue), d / 2.0f)) / (c_sigma_s * c_sigma_s);

                                float energy = exp(-leftTerm - rightTerm);

                                energies[row] += energy;
                                // TODO: when supporting multiple channel blue noise, right term needs treatment
                            }
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

    // return the sum of all energies
    float energySum = 0.0f;
    for (float f : energies)
        energySum += f;
    return energySum;
}

void GenerateBN_Swap(
    std::vector<uint8_t>& pixels,
    size_t width,
    size_t swapTries,
    const char* csvFileName,
    bool limitTo3Sigma,
    float simulatedAnnealingCoolingMultiplier,
    int numSimultaneousSwaps_
)
{
    std::uniform_int_distribution<size_t> dist(0, width*width - 1);
    std::uniform_real_distribution<float> distFloat(0.0f, 1.0f);

    std::uniform_int_distribution<int> distSwapCount(1, numSimultaneousSwaps_);

    FILE* file = nullptr;
    if(csvFileName)
        fopen_s(&file, csvFileName, "w+t");

    // make white noisen and calculate the energy
    std::mt19937 rng(GetRNGSeed());
    std::vector<float> pixelsFloat;
    MakeWhiteNoiseFloat(rng, pixelsFloat, width);
    float pixelsEnergy = limitTo3Sigma ? CalculateEnergy<true>(pixelsFloat, width) : CalculateEnergy<false>(pixelsFloat, width);

    float simulationTemperature = 1.0f *  simulatedAnnealingCoolingMultiplier;

    if (file)
    {
        fprintf(file, "\"Step\",\"Energy\",\"Temperature\"\n");
        fprintf(file, "\"-1\",\"%f\",\"%f\"\n", pixelsEnergy, simulationTemperature);
    }

    // make a copy of the white noise
    std::vector<float> pixelsCopy = pixelsFloat;

    // do swaps to make it more blue
    for (size_t swapTryCount = 0; swapTryCount < swapTries; ++swapTryCount)
    {
        // TODO: keep track of the best energy image and return that instead of the last!
        // TODO: we should let it reach zero temperature before the last sample I think.
        simulationTemperature *= simulatedAnnealingCoolingMultiplier;
        printf("\r%zu / %zu", swapTryCount, swapTries);

        int numSimultaneousSwaps = 1;
        if (numSimultaneousSwaps_ > 1)
            numSimultaneousSwaps = distSwapCount(rng);

        // swap random pixels in the pixels copy
        std::vector<size_t> swaps;
        for (int swapIndex = 0; swapIndex < numSimultaneousSwaps * 2; ++swapIndex)
            swaps.push_back(dist(rng));

        for (int swapIndex = 0; swapIndex < numSimultaneousSwaps; ++swapIndex)
            std::swap(pixelsCopy[swaps[swapIndex * 2]], pixelsCopy[swaps[swapIndex * 2 + 1]]);

        // calculate the new energy
        float newPixelsEnergy = limitTo3Sigma ? CalculateEnergy<true>(pixelsCopy, width) : CalculateEnergy<false>(pixelsCopy, width);

        // if the energy is better, or random chance based on simulation temperature, take it
        if (newPixelsEnergy < pixelsEnergy || (simulationTemperature > 0.0f && distFloat(rng) < simulationTemperature))
        {
            pixelsEnergy = newPixelsEnergy;
            std::swap(pixelsFloat, pixelsCopy);
        }
        // else reverse the swap by doing the same swaps in the reverse order (in case they overlap)
        else
        {
            for (int swapIndex = numSimultaneousSwaps-1; swapIndex >= 0; --swapIndex)
                std::swap(pixelsCopy[swaps[swapIndex * 2]], pixelsCopy[swaps[swapIndex * 2 + 1]]);
        }

        if (file)
            fprintf(file, "\"%zu\",\"%f\",\"%f\"\n", swapTryCount, pixelsEnergy, simulationTemperature);
    }

    if (file)
        fclose(file);

    FromFloat(pixelsFloat, pixels);
    printf("\n");
}

// TODO: could use SIMD for this... that other code does and it seems to run faster. Or put it in notes that it could be improved that way.

// TODO: support multichannel blue noise?
// TODO: profile and optimize