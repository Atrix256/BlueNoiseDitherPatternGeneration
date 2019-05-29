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

float CalculateEnergy(const std::vector<float>& pixels, size_t width)
{
    static const float c_sigma_i = 2.1f;
    static const float c_sigma_s = 1.0f;

    size_t pixelCount = width * width;

    std::vector<std::thread> threads;
    threads.resize(std::thread::hardware_concurrency());
    std::atomic<size_t> atomicp(0);
    std::vector<float> energies(threads.size(), 0.0f);

    // split the work among however many threads are available
    for (size_t threadIndex = 0; threadIndex < threads.size(); ++threadIndex)
    {
        threads[threadIndex] = std::thread(
            [threadIndex, pixelCount, width, &atomicp, &pixels, &energies]()
            {
                // get first pixel to process
                size_t p = atomicp.fetch_add(1);

                // process pixels until we run out
                while (p < pixelCount)
                {
                    float px = float(p % width);
                    float py = float(p / width);
                    float pvalue = pixels[p];

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

                        energies[threadIndex] += energy;
                        // TODO: when supporting multiple channel blue noise, right term needs treatment
                    }

                    // get next pixel to process
                    p = atomicp.fetch_add(1);
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

void GenerateBN_Swap(std::vector<uint8_t>& pixels, size_t width, size_t swapTries, const char* csvFileName)
{
    std::uniform_int_distribution<size_t> dist(0, width*width - 1);

    FILE* file = nullptr;
    if(csvFileName)
        fopen_s(&file, csvFileName, "w+t");

    // make white noisen and calculate the energy
    std::vector<float> pixelsFloat;
    MakeWhiteNoiseFloat(pixelsFloat, width);
    float pixelsEnergy = CalculateEnergy(pixelsFloat, width);

    if (file)
    {
        fprintf(file, "\"Step\",\"Energy\"\n");
        fprintf(file, "\"-1\",\"%f\"\n", pixelsEnergy);
    }

    // make a copy of the white noise
    std::vector<float> pixelsCopy = pixelsFloat;

    // do swaps to make it more blue
    for (size_t swapTryCount = 0; swapTryCount < swapTries; ++swapTryCount)
    {
        printf("\r%zu / %zu", swapTryCount, swapTries);

        // swap two random pixels in the pixels copy
        size_t pixelA = dist(RNG());
        size_t pixelB = dist(RNG());
        std::swap(pixelsCopy[pixelA], pixelsCopy[pixelB]);

        // calculate the new energy
        float newPixelsEnergy = CalculateEnergy(pixelsCopy, width);

        // if the energy is better, take it, else reverse it
        if (newPixelsEnergy < pixelsEnergy)
        {
            pixelsEnergy = newPixelsEnergy;
            std::swap(pixelsFloat, pixelsCopy);
        }
        else
        {
            std::swap(pixelsCopy[pixelA], pixelsCopy[pixelB]);
        }

        if (file)
            fprintf(file, "\"%zu\",\"%f\"\n", swapTryCount, pixelsEnergy);
    }

    if (file)
        fclose(file);

    FromFloat(pixelsFloat, pixels);
}

// TODO: could use SIMD for this... that other code does and it seems to run faster.

// TODO: support multichannel blue noise?
// TODO: profile and optimize

// TODO: other code did 4096 iterations for a 32x32 image
// TODO: other code took higher energy values instead of lower ?!
// TODO: is torroidal distance correct?
