#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <vector>

#include "blur.h"
#include "convert.h"
#include "dft.h"
#include "histogram.h"
#include "misc.h"
#include "whitenoise.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

int main(int argc, char** argv)
{
    std::vector<uint8_t> pixels;
    MakeWhiteNoise(pixels, 256, 256);

    stbi_write_png("out/white.png", 256, 256, 1, pixels.data(), 0);
    WriteHistogram(pixels, "out/white.csv");
   
    std::vector<float> pixelsFloat;
    pixelsFloat.resize(256 * 256);
    for (size_t index = 0, count = 256 * 256; index < count; ++index)
        pixelsFloat[index] = ToFloat(pixels[index]);

    // TODO: calculate size from sigma etc like the other code does
    std::vector<float> pixelsFloatBlurred;
    GaussianBlur(pixelsFloat, pixelsFloatBlurred, 256, 2.1f, 2.1f, 16, 16);

    std::vector<uint8_t> pixelsBlurred;
    pixelsBlurred.resize(256 * 256);
    for (size_t index = 0, count = 256 * 256; index < count; ++index)
        pixelsBlurred[index] = FromFloat<uint8_t>(pixelsFloatBlurred[index]);

    stbi_write_png("out/whiteBlurred.png", 256, 256, 1, pixelsBlurred.data(), 0);
    WriteHistogram(pixelsBlurred, "out/whiteBlurred.csv");

    // TODO: do the repeated blurring / subtraction (HPF) and histogram fix up.  Blur.h exists. need to be able to convert these pixels to float using convert.h functionality

    std::vector<uint8_t> pixelsDFT;
    DFT(pixels, pixelsDFT, 256);
    stbi_write_png("out/white_DFT.png", 256, 256, 1, pixelsDFT.data(), 0);
    DFT(pixelsBlurred, pixelsDFT, 256);
    stbi_write_png("out/whiteBlurred_DFT.png", 256, 256, 1, pixelsDFT.data(), 0);

    WriteHistogram(pixels, "out/white.csv");
    WriteHistogram(pixelsBlurred, "out/whiteBlurred.csv");

    return 0;
}

/*

================== TODO ==================

* sRGB conversion? i think not, but think through that.
* The DFT function is having some problem with DC being huge, so i zero it out for now

================== PLANS ==================

Generation Methods
1) Swap from that paper
2) Leonard's swap
3) high pass filter
4) void and cluster

Do speed testing
Do quality testing: compare DFT for various threshold values.
Maybe also show stuff about individual algorithms, like a DFT after 10, 100, 1000, samples etc.

Also do red noise?
 * unsure what to use it for though...

? Can we use this to make N sheets of blue noise that are blue over space, and each individual pixel is also blue over time?

Links:
* swap paper: https://www.arnoldrenderer.com/research/dither_abstract.pdf
 * and an implementation: https://github.com/joshbainbridge/blue-noise-generator/blob/master/src/main.cpp

*/