#include "blur.h"

static inline float Gaussian(float sigma, float x)
{
    return expf(-(x*x) / (2.0f * sigma*sigma));
}

static inline float GaussianSimpsonIntegration(float sigma, float a, float b)
{
    return
        ((b - a) / 6.0f) *
        (Gaussian(sigma, a) + 4.0f * Gaussian(sigma, (a + b) / 2.0f) + Gaussian(sigma, b));
}

static inline std::vector<float> GaussianKernelIntegrals(float sigma, int taps)
{
    std::vector<float> ret;
    float total = 0.0f;
    for (int i = 0; i < taps; ++i)
    {
        float x = float(i) - float(taps / 2);
        float value = GaussianSimpsonIntegration(sigma, x - 0.5f, x + 0.5f);
        ret.push_back(value);
        total += value;
    }
    // normalize it
    for (unsigned int i = 0; i < ret.size(); ++i)
    {
        ret[i] /= total;
    }
    return ret;
}

static inline const float* GetPixelWrapAround(const std::vector<float>& image, size_t width, int x, int y)
{
    if (x >= (int)width)
    {
        x = x % (int)width;
    }
    else
    {
        while (x < 0)
            x += (int)width;
    }

    if (y >= (int)width)
    {
        y = y % (int)width;
    }
    else
    {
        while (y < 0)
            y += (int)width;
    }

    return &image[(y * width) + x];
}

void GaussianBlur(const std::vector<float>& srcImage, std::vector<float> &destImage, size_t width, float xblursigma, float yblursigma, unsigned int xblursize, unsigned int yblursize)
{
    // allocate space for copying the image for destImage and tmpImage
    destImage.resize(width*width, 0.0f);

    std::vector<float> tmpImage;
    tmpImage.resize(width*width, 0.0f);

    // horizontal blur from srcImage into tmpImage
    {
        auto row = GaussianKernelIntegrals(xblursigma, xblursize);

        int startOffset = -1 * int(row.size() / 2);

        for (int y = 0; y < width; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                float blurredPixel = 0.0f;
                for (unsigned int i = 0; i < row.size(); ++i)
                {
                    const float *pixel = GetPixelWrapAround(srcImage, width, x + startOffset + i, y);
                    blurredPixel += pixel[0] * row[i];
                }

                float *destPixel = &tmpImage[y * width + x];
                destPixel[0] = blurredPixel;
            }
        }
    }

    // vertical blur from tmpImage into destImage
    {
        auto row = GaussianKernelIntegrals(yblursigma, yblursize);

        int startOffset = -1 * int(row.size() / 2);

        for (int y = 0; y < width; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                float blurredPixel = 0.0f;
                for (unsigned int i = 0; i < row.size(); ++i)
                {
                    const float *pixel = GetPixelWrapAround(tmpImage, width, x, y + startOffset + i);
                    blurredPixel += pixel[0] * row[i];
                }

                float *destPixel = &destImage[y * width + x];
                destPixel[0] = blurredPixel;
            }
        }
    }
}