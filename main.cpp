#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <vector>

#include "convert.h"
#include "dft.h"
#include "generatebn_hpf.h"
#include "generatebn_swap.h"
#include "histogram.h"
#include "image.h"
#include "misc.h"
#include "whitenoise.h"
#include "scoped_timer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

void TestMask(const std::vector<uint8_t>& noise, size_t noiseSize, const char* baseFileName)
{
    std::vector<uint8_t> thresholdImage(noise.size());

    for (size_t testIndex = 0; testIndex < THRESHOLD_SAMPLES(); ++testIndex)
    {
        float percent = float(testIndex + 1) / float(THRESHOLD_SAMPLES() + 1);
        uint8_t thresholdValue = FromFloat<uint8_t>(percent);

        for (size_t pixelIndex = 0, pixelCount = noise.size(); pixelIndex < pixelCount; ++pixelIndex)
            thresholdImage[pixelIndex] = noise[pixelIndex] > thresholdValue ? 255 : 0;

        std::vector<uint8_t> thresholdImageDFT;
        DFT(thresholdImage, thresholdImageDFT, noiseSize);

        std::vector<uint8_t> noiseAndDFT;
        size_t noiseAndDFT_width = 0;
        size_t noiseAndDFT_height = 0;
        AppendImageHorizontal(thresholdImage, noiseSize, noiseSize, thresholdImageDFT, noiseSize, noiseSize, noiseAndDFT, noiseAndDFT_width, noiseAndDFT_height);

        char fileName[256];
        sprintf(fileName, "%s_%u.png", baseFileName, unsigned(percent*100.0f));
        stbi_write_png(fileName, int(noiseAndDFT_width), int(noiseAndDFT_height), 1, noiseAndDFT.data(), 0);
    }
}

int main(int argc, char** argv)
{
    // generate some white noise
    {
        ScopedTimer timer("White noise");

        static size_t c_width = 256;

        std::mt19937 rng(GetRNGSeed());
        std::vector<uint8_t> whiteNoise;
        MakeWhiteNoise(rng, whiteNoise, c_width);
        WriteHistogram(whiteNoise, "out/white.histogram.csv");
        std::vector<uint8_t> whiteNoiseDFT;
        DFT(whiteNoise, whiteNoiseDFT, c_width);

        std::vector<uint8_t> noiseAndDFT;
        size_t noiseAndDFT_width = 0;
        size_t noiseAndDFT_height = 0;
        AppendImageHorizontal(whiteNoise, c_width, c_width, whiteNoiseDFT, c_width, c_width, noiseAndDFT, noiseAndDFT_width, noiseAndDFT_height);

        stbi_write_png("out/white.png", int(noiseAndDFT_width), int(noiseAndDFT_height), 1, noiseAndDFT.data(), 0);

        TestMask(whiteNoise, c_width, "out/white");
    }

    // generate blue noise by repeated high pass filtering white noise and fixing up the histogram
    {
        ScopedTimer timer("Blue noise by high pass filtering white noise");

        static size_t c_width = 256;

        std::vector<uint8_t> blueNoise;
        GenerateBN_HPF(blueNoise, c_width);
        WriteHistogram(blueNoise, "out/blueHPF.histogram.csv");
        std::vector<uint8_t> blueNoiseDFT;
        DFT(blueNoise, blueNoiseDFT, c_width);

        std::vector<uint8_t> noiseAndDFT;
        size_t noiseAndDFT_width = 0;
        size_t noiseAndDFT_height = 0;
        AppendImageHorizontal(blueNoise, c_width, c_width, blueNoiseDFT, c_width, c_width, noiseAndDFT, noiseAndDFT_width, noiseAndDFT_height);

        stbi_write_png("out/blueHPF.png", int(noiseAndDFT_width), int(noiseAndDFT_height), 1, noiseAndDFT.data(), 0);

        TestMask(blueNoise, c_width, "out/blueHPF");
    }

    // generate red noise by repeated low pass filtering white noise and fixing up the histogram
    {
        ScopedTimer timer("Red noise by low pass filtering white noise");

        static size_t c_width = 256;

        std::vector<uint8_t> redNoise;
        GenerateBN_HPF(redNoise, c_width, 5, 1.0f, true);
        WriteHistogram(redNoise, "out/redHPF.histogram.csv");
        std::vector<uint8_t> redNoiseDFT;
        DFT(redNoise, redNoiseDFT, c_width);

        std::vector<uint8_t> noiseAndDFT;
        size_t noiseAndDFT_width = 0;
        size_t noiseAndDFT_height = 0;
        AppendImageHorizontal(redNoise, c_width, c_width, redNoiseDFT, c_width, c_width, noiseAndDFT, noiseAndDFT_width, noiseAndDFT_height);

        stbi_write_png("out/redHPF.png", int(noiseAndDFT_width), int(noiseAndDFT_height), 1, noiseAndDFT.data(), 0);

        TestMask(redNoise, c_width, "out/redHPF");
    }

    // generate blue noise by swapping white noise pixels to make it more blue
    {
        ScopedTimer timer("Blue noise by swapping white noise");

        static size_t c_width = 32;
        static size_t c_numSwaps = 4096;

        std::vector<uint8_t> blueNoise;
        GenerateBN_Swap(blueNoise, c_width, c_numSwaps, "out/blueSwap.data.csv", true, 0.0f);
        WriteHistogram(blueNoise, "out/blueSwap.histogram.csv");
        std::vector<uint8_t> blueNoiseDFT;
        DFT(blueNoise, blueNoiseDFT, c_width);

        std::vector<uint8_t> noiseAndDFT;
        size_t noiseAndDFT_width = 0;
        size_t noiseAndDFT_height = 0;
        AppendImageHorizontal(blueNoise, c_width, c_width, blueNoiseDFT, c_width, c_width, noiseAndDFT, noiseAndDFT_width, noiseAndDFT_height);

        stbi_write_png("out/blueSwap.png", int(noiseAndDFT_width), int(noiseAndDFT_height), 1, noiseAndDFT.data(), 0);

        TestMask(blueNoise, c_width, "out/blueSwap");
    }

    // generate blue noise by swapping white noise pixels to make it more blue - with Simulated Annealing
    {
        ScopedTimer timer("Blue noise by swapping white noise - with SA");

        static size_t c_width = 32;
        static size_t c_numSwaps = 4096;

        std::vector<uint8_t> blueNoise;
        GenerateBN_Swap(blueNoise, c_width, c_numSwaps, "out/blueSwapSA.data.csv", true, 0.99f);
        WriteHistogram(blueNoise, "out/blueSwapSA.histogram.csv");
        std::vector<uint8_t> blueNoiseDFT;
        DFT(blueNoise, blueNoiseDFT, c_width);

        std::vector<uint8_t> noiseAndDFT;
        size_t noiseAndDFT_width = 0;
        size_t noiseAndDFT_height = 0;
        AppendImageHorizontal(blueNoise, c_width, c_width, blueNoiseDFT, c_width, c_width, noiseAndDFT, noiseAndDFT_width, noiseAndDFT_height);

        stbi_write_png("out/blueSwapSA.png", int(noiseAndDFT_width), int(noiseAndDFT_height), 1, noiseAndDFT.data(), 0);

        TestMask(blueNoise, c_width, "out/blueSwapSA");
    }

    system("pause");

    return 0;
}

/*

================== TODO ==================

For Swap Method...
2) try with simulated annealing: decreasing temperature is a probability to swap regardless of score
3) try with metropolis: same as SA but use how much worse it is probability
4) Metropolis and SA: combine them by multiplying probabilities
* keep track of best seen and report that instead of last.
5) 5th blue noise technique - best candidate algorithm. for a specific value, choose some number of them at random, pick whatever one is farthest from existing points
* Round robin give each value a sample before moving onto sample 2,3,etc
* Could also try having that energy function be what scores candidates instead of distance. Only consider pixels with values set already

* could try multiple swaps at once. maybe start with a larger number then decrease the swap count as time goes on?

* other todo's in other files (i think just generatebn_swap.cpp)

? does the thresholding constraint make blue noise that's worse for the non thresholding case?



================== PLANS ==================

Generation Methods
1) Swap from that paper
2) Leonard's swap
3) high pass filter   - DONE
4) void and cluster

Do speed testing
Do quality testing: compare DFT for various threshold values.
Maybe also show stuff about individual algorithms, like a DFT after 10, 100, 1000, samples etc.

Also do red noise?
 * unsure what to use it for though...

? Can we use this to make N sheets of blue noise that are blue over space, and each individual pixel is also blue over time?


================== NOTES ==================

 * blue noise dither pattern has 2 uses: screen space noise (needs to be blue) and thresholding (subsets need to be blue)
  * 1st is for like AO ray perterbation & ray march offsets
  * 2nd is for stochastic alpha and dithering before quantization

* swap paper:
 * could speed up swap algorithm with SIMD. GPU probably too.
 * could try swapping multiple at once til it plateaus then decrease it.
 * could limit p's to being within 3 std dev of swapped pixels. calculate score before and after swap for region each time you do a comparison.
  * not compatible with some things because you don't know overall image score, so can't keep best. sounds fast though.
  * score calculation speed no longer resolution dependent, but you do still need more swaps for larger images.
 * check out the blog folder for some more stuff for swap algorithm.

* The DFT function is having some problem with DC being huge, so i zero it out for now
 * meh...

================== LINKS ==================

* repeated LPF: https://blog.demofox.org/2017/10/25/transmuting-white-noise-to-blue-red-green-purple/

* swap paper: https://www.arnoldrenderer.com/research/dither_abstract.pdf
 * and an implementation: https://github.com/joshbainbridge/blue-noise-generator/blob/master/src/main.cpp

*/