#include "blur.h"
#include "convert.h"
#include "generatebn_hpf.h"
#include "whitenoise.h"

static void NormalizeHistogram(std::vector<float>& image, size_t width)
{
    struct SHistogramHelper
    {
        float value;
        size_t pixelIndex;
    };
    static std::vector<SHistogramHelper> pixels;
    pixels.resize(width*width);

    // put all the pixels into the array
    for (size_t i = 0, c = pixels.size(); i < c; ++i)
    {
        pixels[i].value = image[i];
        pixels[i].pixelIndex = i;
    }

    // shuffle the pixels to randomly order ties. not as big a deal with floating point pixel values though
    std::shuffle(pixels.begin(), pixels.end(), RNG());

    // sort the pixels by value
    std::sort(
        pixels.begin(),
        pixels.end(),
        [](const SHistogramHelper& a, const SHistogramHelper& b)
        {
            return a.value < b.value;
        }
    );

    // use the pixel's place in the array as the new value, and write it back to the image
    for (size_t i = 0, c = width * width; i < c; ++i)
    {
        float value = float(i) / float(c - 1);
        image[pixels[i].pixelIndex] = value;
    }
}

void GenerateBN_HPF(std::vector<uint8_t>& blueNoise, size_t width, size_t numPasses, float sigma, bool makeRed)
{
    // first make white noise
    std::vector<uint8_t> pixels;
    MakeWhiteNoise(pixels, width);

    // convert from uint8 to float
    std::vector<float> pixelsFloat;
    ToFloat(pixels, pixelsFloat);

    // repeatedly high pass filter and histogram fixup
    std::vector<float> pixelsFloatLowPassed;
    for (size_t index = 0; index < numPasses; ++index)
    {
        GaussianBlur(pixelsFloat, pixelsFloatLowPassed, width, sigma);

        if (!makeRed)
        {
            // a blur is a low pass filter, and a high pass filter is the signal minus the low pass filtered signa.
            for (size_t pixelIndex = 0, pixelCount = pixelsFloat.size(); pixelIndex < pixelCount; ++pixelIndex)
                pixelsFloat[pixelIndex] -= pixelsFloatLowPassed[pixelIndex];
        }
        else
        {
            // to make red noise, just use the low pass filter data
            for (size_t pixelIndex = 0, pixelCount = pixelsFloat.size(); pixelIndex < pixelCount; ++pixelIndex)
                pixelsFloat[pixelIndex] = pixelsFloatLowPassed[pixelIndex];
        }

        // Do a histogram fixup
        NormalizeHistogram(pixelsFloat, width);
    }

    // convert back to uint8
    FromFloat(pixelsFloat, blueNoise);
}