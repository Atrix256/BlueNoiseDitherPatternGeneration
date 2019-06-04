#include "convert.h"
#include "generatebn_paniq.h"
#include "whitenoise.h"

#include <thread>
#include <atomic>
#include <array>

#define R2 19
#define SIGMA 1.414f
#define M_PI 3.14159265359f

typedef std::array<float, 2> vec2;
typedef std::array<float, 3> vec3;
typedef std::array<float, 4> vec4;
typedef std::array<int, 2> ivec2;

template <size_t N>
std::array<float, N> operator* (const std::array<float, N>& A, const std::array<float, N>& B)
{
    std::array<float, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] * B[i];
    return ret;
}

template <typename T, size_t N>
std::array<T, N> operator+ (const std::array<T, N>& A, const std::array<T, N>& B)
{
    std::array<T, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] + B[i];
    return ret;
}

template <size_t N>
std::array<int, N> operator^ (const std::array<int, N>& A, const std::array<int, N>& B)
{
    std::array<int, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] ^ B[i];
    return ret;
}

template <size_t N>
std::array<int, N> operator% (const std::array<int, N>& A, const std::array<int, N>& B)
{
    std::array<int, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] % B[i];
    return ret;
}

template <size_t N>
std::array<float, N> operator+ (const std::array<float, N>& A, float B)
{
    std::array<float, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] + B;
    return ret;
}

template <size_t N>
std::array<float, N> operator* (const std::array<float, N>& A, float B)
{
    std::array<float, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] * B;
    return ret;
}

template <size_t N>
std::array<float, N> fract(const std::array<float, N>& A)
{
    std::array<float, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = std::fmodf(A[i], 1.0f);
    return ret;
}

float fract(float A)
{
    return std::fmodf(A, 1.0f);
}

float max(float A, float B)
{
    return std::max(A, B);
}

template <size_t N>
float dot(const std::array<float, N>& A, const std::array<float, N>& B)
{
    float ret = 0.0f;
    for (size_t i = 0; i < N; ++i)
        ret += A[i] * B[i];
    return ret;
}

template <size_t N>
float length(const std::array<float, N>& A)
{
    float lengthSq = 0.0f;
    for (size_t i = 0; i < N; ++i)
        lengthSq += A[i] * A[i];
    return std::sqrtf(lengthSq);
}

float clamp(float value, float min, float max)
{
    if (value <= min)
        return min;
    else if (value >= max)
        return max;
    else
        return value;
}

std::array<float, 3> yzx(const std::array<float, 3>& A)
{
    return { A[1], A[2], A[0] };
}

std::array<float, 2> xx(const std::array<float, 3>& A)
{
    return { A[0], A[0] };
}

std::array<float, 2> yz(const std::array<float, 3>& A)
{
    return { A[1], A[2] };
}

std::array<float, 2> zy(const std::array<float, 3>& A)
{
    return { A[2], A[1] };
}

vec2 hash21(float p)
{
    vec3 p3 = fract(vec3{ p,p,p } *vec3{ 0.1031f, 0.1030f, 0.0973f });
    p3 = p3 + dot(p3, yzx(p3) + 19.19f);
    return fract((xx(p3) + yz(p3))*zy(p3));
}

float hash13(vec3 p3)
{
    p3 = fract(p3 * 0.1031f);
    p3 =  p3 + dot(p3, yzx(p3) + 19.19f);
    return fract((p3[0] + p3[1]) * p3[2]);
}

float gaussian(float x, float sigma) {
    float h0 = x / sigma;
    float h = h0 * h0 * -0.5f;
    float a = 1.0f / float(sigma * sqrt(2.0f * M_PI));
    return a * exp(h);
}

float distf(float v, float x) {
    return 1.0f - x;
}

vec2 quantify_error(const std::vector<float>& oldNoise, size_t oldNoiseWidth, ivec2 p, ivec2 sz, float val0, float val1)
{
    float Rf = float(R2) / 2.0;
    int R = int(Rf);
    float has0 = 0.0;
    float has1 = 0.0;
    float w = 0.0;

    for (int sy = -R; sy < R2; ++sy) {
        for (int sx = -R; sx < R2; ++sx) {
            float d = length(vec2{ float(sx), float(sy) });
            if ((d > Rf) || ((sx == 0) && (sy == 0)))
                continue;
            ivec2 t = (p + ivec2{ sx, sy } +sz) % sz;
            float v = oldNoise[t[1] * oldNoiseWidth + t[0]];

            float dist0 = abs(v - val0);
            float dist1 = abs(v - val1);

            float q = gaussian(d, SIGMA);

            w += q;
            has0 += distf(val0, dist0) * q;
            has1 += distf(val1, dist1) * q;
        }
    }

    vec2 result = vec2{ has0 / w, has1 / w };
    //result = result * result;
    return result;
}

template <bool MAKE_BLUE_NOISE>
void mainImage(float& fragColor, const vec2& fragCoord, const ivec2& sz, size_t iFrame, const std::vector<float>& oldNoise, size_t oldNoiseWidth)
{
    ivec2 p0 = ivec2{ int(fragCoord[0]),int(fragCoord[1]) };
    vec2 maskf = hash21(float(iFrame));
    int M = 60 * 60;
    int F = (iFrame % M);
    float framef = float(F) / float(M);
    float chance_limit = 0.5f;
    float force_limit = 1.0f - clamp(framef * 8.0f, 0.0f, 1.0f);
    force_limit = force_limit * force_limit;
    force_limit = force_limit * force_limit;/*
    if (F == 0) {
        int c = (p0[0] * 61 + p0[1]) % 256;
        fragColor = float(c) / 255.0f;
    }
    else*/ {
        vec2 temp = vec2{ float(sz[0]), float(sz[1]) } *maskf + vec2{ float(sz[0]), float(sz[1]) } *maskf * framef;

        ivec2 mask = ivec2{ int(temp[0]), int(temp[1]) };
        ivec2 p1 = (p0 ^ mask) % sz;
        ivec2 pp0 = (p1 ^ mask) % sz;

        float chance0 = hash13(vec3{ float(p0[0]), float(p0[1]), float(iFrame) });
        float chance1 = hash13(vec3{ float(p1[0]), float(p1[1]), float(iFrame) });
        float chance = max(chance0, chance1);

        float v0 = oldNoise[p0[1] * oldNoiseWidth + p0[0]];
        float v1 = oldNoise[p1[1] * oldNoiseWidth + p1[0]];

        vec2 s0_x0 = quantify_error(oldNoise, oldNoiseWidth, p0, sz, v0, v1);
        vec2 s1_x1 = quantify_error(oldNoise, oldNoiseWidth, p1, sz, v1, v0);

        float err_s = s0_x0[0] + s1_x1[0];
        float err_x = s0_x0[1] + s1_x1[1];

        float p = v0;
        if (pp0 == p0) {

            if (MAKE_BLUE_NOISE)
            {
                if (/*(chance < force_limit) ||*/ ((chance < chance_limit) && (err_x < err_s))) {
                    p = v1;
                }
            }
            else
            {
                if (/*(chance < force_limit) ||*/ ((chance < chance_limit) && (err_x > err_s))) {
                    p = v1;
                }
            }
        }
        fragColor = p;
    }
}

void GenerateBN_Paniq(
    std::vector<uint8_t>& blueNoise,
    size_t width,
    size_t iterations,
    bool makeBlueNoise
)
{
    // start with some white noise
    std::mt19937 rng(GetRNGSeed());
    std::vector<float> noise, noise2;
    MakeWhiteNoiseFloat(rng, noise, width);
    noise2 = noise;

    // do multiple iterations of this: reading from noise and writing to noise2
    for (size_t iteration = 0; iteration < iterations; ++iteration)
    {
        printf("\r%i%%", int(100.0f*float(iteration) / float(iterations)));

        // each iteration, swap them because what was previously the better noise is now the lesser noise compared to the next iteration noise
        std::swap(noise, noise2);

        // run the pixel shader per pixel
        // split the work among however many threads are available
        std::vector<std::thread> threads;
        threads.resize(std::thread::hardware_concurrency());
        std::atomic<size_t> atomicrow(0);
        for (size_t threadIndex = 0; threadIndex < threads.size(); ++threadIndex)
        {
            threads[threadIndex] = std::thread(
                [width, iteration, makeBlueNoise, &atomicrow, &noise, &noise2]()
                {
                    // get first row to process
                    size_t row = atomicrow.fetch_add(1);

                    // process rows until we run out
                    while (row < width)
                    {
                        float* dest = &noise2[row*width];

                        for (size_t ix = 0; ix < width; ++ix)
                        {
                            float pixelOut;
                            if(makeBlueNoise)
                                mainImage<true>(pixelOut, vec2{ float(ix), float(row) }, ivec2{ int(width), int(width) }, iteration, noise, width);
                            else
                                mainImage<false>(pixelOut, vec2{ float(ix), float(row) }, ivec2{ int(width), int(width) }, iteration, noise, width);
                            dest[ix] = pixelOut;
                        }

                        // go to the next row
                        row = atomicrow.fetch_add(1);
                    }
                }
            );
        }

        // wait for all threads to be done
        for (std::thread& t : threads)
            t.join();
    }
    printf("\n");

    // convert from float to U8 into the blue noise array
    FromFloat(noise2, blueNoise);
}
