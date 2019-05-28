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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

int main(int argc, char** argv)
{
    static size_t c_width = 32;

    /*
    // generate some white noise
    {
        std::vector<uint8_t> whiteNoise;
        MakeWhiteNoise(whiteNoise, c_width);
        stbi_write_png("out/white.png", int(c_width), int(c_width), 1, whiteNoise.data(), 0);
        WriteHistogram(whiteNoise, "out/white.histogram.csv");
        std::vector<uint8_t> whiteNoiseDFT;
        DFT(whiteNoise, whiteNoiseDFT, c_width);
        stbi_write_png("out/white.DFT.png", int(c_width), int(c_width), 1, whiteNoiseDFT.data(), 0);
    }

    // generate blue noise by repeated high pass filtering white noise and fixing up the histogram
    {
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
        std::vector<uint8_t> redNoise;
        GenerateBN_HPF(redNoise, c_width, 5, 1.0f, true);
        stbi_write_png("out/redHPF.png", int(c_width), int(c_width), 1, redNoise.data(), 0);
        WriteHistogram(redNoise, "out/redHPF.histogram.csv");
        std::vector<uint8_t> redNoiseDFT;
        DFT(redNoise, redNoiseDFT, c_width);
        stbi_write_png("out/redHPF.DFT.png", int(c_width), int(c_width), 1, redNoiseDFT.data(), 0);
    }
    */

    // generate blue noise by swapping white noise pixels to make it more blue
    {
        std::vector<uint8_t> blueNoise;
        GenerateBN_Swap(blueNoise, c_width, 4096);
        stbi_write_png("out/blueSwap.png", int(c_width), int(c_width), 1, blueNoise.data(), 0);
        WriteHistogram(blueNoise, "out/blueSwap.histogram.csv");
        std::vector<uint8_t> blueNoiseDFT;
        DFT(blueNoise, blueNoiseDFT, c_width);
        stbi_write_png("out/blueSwap.DFT.png", int(c_width), int(c_width), 1, blueNoiseDFT.data(), 0);
    }

    return 0;
}

/*

================== TODO ==================

* make images and DFT images be put together as one image
* The DFT function is having some problem with DC being huge, so i zero it out for now

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

Links:
* repeated LPF: https://blog.demofox.org/2017/10/25/transmuting-white-noise-to-blue-red-green-purple/

* swap paper: https://www.arnoldrenderer.com/research/dither_abstract.pdf
 * and an implementation: https://github.com/joshbainbridge/blue-noise-generator/blob/master/src/main.cpp

*/