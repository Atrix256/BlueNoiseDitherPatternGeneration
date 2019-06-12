#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <vector>

#include "convert.h"
#include "dft.h"
#include "generatebn_hpf.h"
#include "generatebn_paniq.h"
#include "generatebn_swap.h"
#include "generatebn_void_cluster.h"
#include "histogram.h"
#include "image.h"
#include "misc.h"
#include "whitenoise.h"
#include "scoped_timer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

void TestMask(const std::vector<uint8_t>& noise, size_t noiseSize, const char* baseFileName)
{
    std::vector<uint8_t> thresholdImage(noise.size());

    for (int thresholdValue_ = 0; thresholdValue_ < 256; ++thresholdValue_)
    //for (size_t testIndex = 0; testIndex < THRESHOLD_SAMPLES(); ++testIndex)
    {
        //float percent = float(testIndex + 1) / float(THRESHOLD_SAMPLES() + 1);
        uint8_t thresholdValue = uint8_t(thresholdValue_);// FromFloat<uint8_t>(percent);

        for (size_t pixelIndex = 0, pixelCount = noise.size(); pixelIndex < pixelCount; ++pixelIndex)
            thresholdImage[pixelIndex] = noise[pixelIndex] > thresholdValue ? 255 : 0;

        std::vector<uint8_t> thresholdImageDFT;
        DFT(thresholdImage, thresholdImageDFT, noiseSize);

        std::vector<uint8_t> noiseAndDFT;
        size_t noiseAndDFT_width = 0;
        size_t noiseAndDFT_height = 0;
        AppendImageHorizontal(thresholdImage, noiseSize, noiseSize, thresholdImageDFT, noiseSize, noiseSize, noiseAndDFT, noiseAndDFT_width, noiseAndDFT_height);

        char fileName[256];
        sprintf(fileName, "%s_%u.png", baseFileName, thresholdValue);
        stbi_write_png(fileName, int(noiseAndDFT_width), int(noiseAndDFT_height), 1, noiseAndDFT.data(), 0);
    }
}

void TestNoise(const std::vector<uint8_t>& noise, size_t noiseSize, const char* baseFileName)
{
    char fileName[256];
    sprintf(fileName, "%s.histogram.csv", baseFileName);

    WriteHistogram(noise, fileName);
    std::vector<uint8_t> noiseDFT;
    DFT(noise, noiseDFT, noiseSize);

    std::vector<uint8_t> noiseAndDFT;
    size_t noiseAndDFT_width = 0;
    size_t noiseAndDFT_height = 0;
    AppendImageHorizontal(noise, noiseSize, noiseSize, noiseDFT, noiseSize, noiseSize, noiseAndDFT, noiseAndDFT_width, noiseAndDFT_height);

    sprintf(fileName, "%s.png", baseFileName);
    stbi_write_png(fileName, int(noiseAndDFT_width), int(noiseAndDFT_height), 1, noiseAndDFT.data(), 0);

    TestMask(noise, noiseSize, baseFileName);
}

int main(int argc, char** argv)
{
    /*
    // generate some white noise
    {
        ScopedTimer timer("White noise");

        static size_t c_width = 256;

        std::mt19937 rng(GetRNGSeed());
        std::vector<uint8_t> noise;
        MakeWhiteNoise(rng, noise, c_width);

        TestNoise(noise, c_width, "out/white");
    }

    // generate blue noise by repeated high pass filtering white noise and fixing up the histogram
    {
        ScopedTimer timer("Blue noise by high pass filtering white noise");

        static size_t c_width = 256;

        std::vector<uint8_t> noise;
        GenerateBN_HPF(noise, c_width);

        TestNoise(noise, c_width, "out/blueHPF");
    }

    // generate red noise by repeated low pass filtering white noise and fixing up the histogram
    {
        ScopedTimer timer("Red noise by low pass filtering white noise");

        static size_t c_width = 256;

        std::vector<uint8_t> noise;
        GenerateBN_HPF(noise, c_width, 5, 1.0f, true);

        TestNoise(noise, c_width, "out/redHPF");
    }
    */

    // generate blue noise using void and cluster
    {
        ScopedTimer timer("Blue noise by void and cluster");

        static size_t c_width = 256;

        std::vector<uint8_t> noise;
        GenerateBN_Void_Cluster(noise, c_width, "out/blueVC_1_");
        TestNoise(noise, c_width, "out/blueVC_1_");
    }

    // load a blue noise texture
    {
        ScopedTimer timer("Blue noise by void and cluster from loading the texture");

        int width, height, channels;
        uint8_t* image = stbi_load("bluenoise256.png", &width, &height, &channels, 0);

        std::vector<uint8_t> noise;
        noise.reserve(width*height);
        for (int i = 0; i < width*height; ++i)
            noise.push_back(image[i*channels]);

        stbi_image_free(image);

        TestNoise(noise, width, "out/blueVC_2_");
    }

    // TODO: profile and multithread void and cluster? see how it compares to paniq's timing!
    // TODO: paniq was like 30s for 256x256. V & C is like 110 seconds. his is parallelizable, V & C isn't.
    // TODO: make void and cluster thing be bigger

    // TODO: temp!
    system("pause");
    return 0;

    // generate blue noise by using paniq's technique
    {
        ScopedTimer timer("Blue noise by paniq");

        static size_t c_width = 256;
        static size_t c_iterations = 120;

        std::vector<uint8_t> noise;
        GenerateBN_Paniq(noise, c_width, c_iterations, true);

        TestNoise(noise, c_width, "out/bluePaniq");
    }

    // generate blue noise by using paniq's technique
    {
        ScopedTimer timer("Red noise by paniq");

        static size_t c_width = 256;
        static size_t c_iterations = 120;

        std::vector<uint8_t> noise;
        GenerateBN_Paniq(noise, c_width, c_iterations, false);

        TestNoise(noise, c_width, "out/redPaniq");
    }

    // generate blue noise by swapping white noise pixels to make it more blue
    {
        ScopedTimer timer("Blue noise by swapping white noise");

        static size_t c_width = 32;
        static size_t c_numSwaps = 4096;

        std::vector<uint8_t> noise;
        GenerateBN_Swap(noise, c_width, c_numSwaps, "out/blueSwap1.data.csv", true, 0.0f, 1, false, true);

        TestNoise(noise, c_width, "out/blueSwap1");
    }

    // generate blue noise by swapping white noise pixels to make it more blue. 1-5 swaps
    {
        ScopedTimer timer("Blue noise by swapping white noise. 1-5 swaps.");

        static size_t c_width = 32;
        static size_t c_numSwaps = 4096;

        std::vector<uint8_t> noise;
        GenerateBN_Swap(noise, c_width, c_numSwaps, "out/blueSwap5.data.csv", true, 0.0f, 5, false, true);

        TestNoise(noise, c_width, "out/blueSwap5");
    }

    // generate blue noise by swapping white noise pixels to make it more blue. 1-10 swaps
    {
        ScopedTimer timer("Blue noise by swapping white noise. 1-10 swaps.");

        static size_t c_width = 32;
        static size_t c_numSwaps = 4096;

        std::vector<uint8_t> noise;
        GenerateBN_Swap(noise, c_width, c_numSwaps, "out/blueSwap10.data.csv", true, 0.0f, 10, false, true);

        TestNoise(noise, c_width, "out/blueSwap10");
    }

    // generate red noise by swapping white noise pixels to make it more red. 1-10 swaps
    {
        ScopedTimer timer("Red noise by swapping white noise. 1-10 swaps.");

        static size_t c_width = 32;
        static size_t c_numSwaps = 4096;

        std::vector<uint8_t> noise;
        GenerateBN_Swap(noise, c_width, c_numSwaps, "out/redSwap10.data.csv", true, 0.0f, 10, false, false);

        TestNoise(noise, c_width, "out/redSwap10");
    }

    // generate blue noise by swapping white noise pixels to make it more blue - with Simulated Annealing
    {
        ScopedTimer timer("Blue noise by swapping white noise - with SA");

        static size_t c_width = 32;
        static size_t c_numSwaps = 4096;

        std::vector<uint8_t> noise;
        GenerateBN_Swap(noise, c_width, c_numSwaps, "out/blueSwapSA.data.csv", true, 0.99f, 1, false, true);

        TestNoise(noise, c_width, "out/blueSwapSA");
    }

    // generate blue noise by swapping white noise pixels to make it more blue - with metropolis algorithm and 1-10 swaps
    {
        ScopedTimer timer("Blue noise by swapping white noise - with metropolis and 1-10 swaps");

        static size_t c_width = 32;
        static size_t c_numSwaps = 4096;

        std::vector<uint8_t> noise;
        GenerateBN_Swap(noise, c_width, c_numSwaps, "out/blueSwapMet.data.csv", true, 1.0f, 10, false, true);

        TestNoise(noise, c_width, "out/blueSwapMet");
    }

    system("pause");

    return 0;
}

/*

* maybe could multithread LUT writing & generation since not doing DFT/IDFT?
 * could maybe do DFT/IDFT??!

* maybe compare your thresholding and DFT to the "free blue noise textures" results?

* make a blog post on void and cluster by itself.
* big optimizations:
 1) limit to 3 sigma
 2) use a LUT where you write your values.
* also of note: compare it to mitchell's best candidate for speed and quality of generating sample points!
 * mention how "initial binary pattern" makes blue noise distributed points, and "phase 1" makes it prorgessive.
 * phase 1 could make other blue noise-esque distributed points progressive.

================== TODO ==================

* probably can boost up the size of initial binary pattern again to width*width/4?

* have V&C use LUT for initial binary pattern and compare time and quality vs mitchells
 * 48 seconds total 256x256 not using lut for initial binary pattern setup
 * 35 seconds for initial pattern. 364 seconds for phase 1
 * with LUT -> 640 ms for initial binary pattern, 400 ms for phase 1
  * vs 1800ms for mitchell's best candidate


Paniq in regards to my LUT void and cluster on gpu
"there's often a parallelizable solution somewhere. in this case it looks like a pyramid histogram would once again save the day (that's the GPU version of map-reduce)"


* Without LUT, but 3 sigma. 30 seconds for 64x64
 * adding LUT to phase 1 dropped it to 23 seconds
 * adding LUT to phase 2 dropped it to 16 seconds.
 * adding LUT to phase 3 dropped it to 3.5 seconds.
 ! NOTE that LUT is single threaded!! The old way was multithreade


? can void and cluster make red noise?
 * maybe if we randomize ties it can

For Swap Method...
5) 5th blue noise technique - best candidate algorithm. for a specific value, choose some number of them at random, pick whatever one is farthest from existing points
* Round robin give each value a sample before moving onto sample 2,3,etc
* Could also try having that energy function be what scores candidates instead of distance. Only consider pixels with values set already

* void and cluster

* other todo's in other files (i think just generatebn_swap.cpp)

? does the thresholding constraint make blue noise that's worse for the non thresholding case?



================== PLANS ==================

Generation Methods
1) Swap from that paper - DONE
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

* for void and cluster, it'd be neat to show the evolution of the starting pattern with the red, green, yellow and white dots. Maybe a gif.
* show timing of void and cluster as it scales up.  Show it for small resolutions not limiting it to 3 sigma, then crank it up when limiting to 3 sigma.
* could show LUT images too
* Without LUT, but 3 sigma and multithreaded. 30 seconds for 64x64. with LUT and single threaded drops it to 3.5 seconds!
* with LUTs, 115 seconds for 256x256. 100 for initial binary pattern. 2 for phase1, 5 for phase2, 7 for phase 3.
 * If we can generate blue noise samples, that replaces "initial binary pattern" step.
 * If we can generate progressive blue noise samples, that also replaces phase 1.
 * I tried Mitchell's best candidate algorithm to replace these two steps and it is terrible.
  * until i used a grid to speed up "find closest point".
  * 256x256 with width*width/4 initial points.  using initial/phase 1: 55s total.  using mitchell's best candidate: 15s total!
  * Mitchell's has a faint + sign in the middle of the DFT but meh whatever.
 * I'm guessing that CCVD would do decently too at least for the "initial binary pattern" step. Phase 1 would still need to happen.


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


 
 * simulated annealing as i implemented it seems to have failed: sometimes taking worse results didn't help break out of any local minima
 * metropolis didn't help either: it's a different way of choosing worse results some of the time. didn't help break out of any local minima.
 * what does seem to help is sometimes choosing more than 1 swap.  It's possible that maybe the # of swaps tested could start out high and decrease over time.
  * I didn't try that
  * a good test would be to graph the success vs failures of # of swaps over time.
  * if higher counts worked better in the beginning but failed more towards the end, it'd be more efficient to use lower counts towards the end.


* Paniq's technique is really fast and does well when thresholded too!
 * the histogram is also balanced amazingly...
 * need to understand how it does what it does better

================== LINKS ==================

* repeated LPF: https://blog.demofox.org/2017/10/25/transmuting-white-noise-to-blue-red-green-purple/

* swap paper: https://www.arnoldrenderer.com/research/dither_abstract.pdf
 * and an implementation: https://github.com/joshbainbridge/blue-noise-generator/blob/master/src/main.cpp

paniq's thing, inspired by it but meant for use on the GPU. Implemented in shadertoy.
* https://www.shadertoy.com/view/XtdyW2 
* https://twitter.com/paniq/status/1134087386651082756?s=12
* it converges in 20 seconds (60*20 = 1200 frames).  It can actually convert in 2 seconds (120 frames) if you start with good white noise or don't care if an edge case isn't great.
* I haven't checked the DFT but to the eye it looks good. (should check DFT!)

* void and cluster: http://cv.ulichney.com/papers/1993-void-cluster.pdf
 * and this: http://momentsingraphics.de/?p=127#citation-ulichney93

*/