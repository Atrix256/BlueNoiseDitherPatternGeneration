#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <vector>

#include "dft.h"
#include "generatebn_hpf.h"
#include "generatebn_swap.h"
#include "histogram.h"
#include "misc.h"
#include "whitenoise.h"
#include "scoped_timer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

int main(int argc, char** argv)
{
    // generate some white noise
    {
        ScopedTimer timer("White noise");

        static size_t c_width = 256;

        std::mt19937 rng(GetRNGSeed());
        std::vector<uint8_t> whiteNoise;
        MakeWhiteNoise(rng, whiteNoise, c_width);
        stbi_write_png("out/white.png", int(c_width), int(c_width), 1, whiteNoise.data(), 0);
        WriteHistogram(whiteNoise, "out/white.histogram.csv");
        std::vector<uint8_t> whiteNoiseDFT;
        DFT(whiteNoise, whiteNoiseDFT, c_width);
        stbi_write_png("out/white.DFT.png", int(c_width), int(c_width), 1, whiteNoiseDFT.data(), 0);
    }

    // generate blue noise by repeated high pass filtering white noise and fixing up the histogram
    {
        ScopedTimer timer("Blue noise by high pass filtering white noise");

        static size_t c_width = 256;

        std::vector<uint8_t> blueNoise;
        GenerateBN_HPF(blueNoise, c_width);
        stbi_write_png("out/blueHPF.png", int(c_width), int(c_width), 1, blueNoise.data(), 0);
        WriteHistogram(blueNoise, "out/blueHPF.histogram.csv");
        std::vector<uint8_t> blueNoiseDFT;
        DFT(blueNoise, blueNoiseDFT, c_width);
        stbi_write_png("out/blueHPF.DFT.png", int(c_width), int(c_width), 1, blueNoiseDFT.data(), 0);
    }

    // generate red noise by repeated low pass filtering white noise and fixing up the histogram
    {
        ScopedTimer timer("Red noise by low pass filtering white noise");

        static size_t c_width = 256;

        std::vector<uint8_t> redNoise;
        GenerateBN_HPF(redNoise, c_width, 5, 1.0f, true);
        stbi_write_png("out/redHPF.png", int(c_width), int(c_width), 1, redNoise.data(), 0);
        WriteHistogram(redNoise, "out/redHPF.histogram.csv");
        std::vector<uint8_t> redNoiseDFT;
        DFT(redNoise, redNoiseDFT, c_width);
        stbi_write_png("out/redHPF.DFT.png", int(c_width), int(c_width), 1, redNoiseDFT.data(), 0);
    }

    // generate blue noise by swapping white noise pixels to make it more blue
    {
        ScopedTimer timer("Blue noise by swapping white noise");

        static size_t c_width = 32;
        static size_t c_numSwaps = 4096;

        std::vector<uint8_t> blueNoise;
        GenerateBN_Swap(blueNoise, c_width, c_numSwaps, "out/blueSwap.data.csv", true);
        stbi_write_png("out/blueSwap.png", int(c_width), int(c_width), 1, blueNoise.data(), 0);
        WriteHistogram(blueNoise, "out/blueSwap.histogram.csv");
        std::vector<uint8_t> blueNoiseDFT;
        DFT(blueNoise, blueNoiseDFT, c_width);
        stbi_write_png("out/blueSwap.DFT.png", int(c_width), int(c_width), 1, blueNoiseDFT.data(), 0);
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
* different results each time due to thread stuff.
 * maybe have atomic give out rows of pixels, and have them sum their scores into a row.
 * after the threads are done, it can single thread sum the rows.
 * this should be deterministic.

* other todo's in other files (i think just generatebn_swap.cpp)

* make images and DFT images be put together as one image
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