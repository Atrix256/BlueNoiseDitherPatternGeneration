#pragma once

#include <random>

std::mt19937& RNG()
{
    static std::random_device rd;
    static std::seed_seq fullSeed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
    static std::mt19937 rng(fullSeed);
    return rng;
}

template <typename T>
T RandomValue()
{
    std::uniform_int_distribution<T> dist(0, std::numeric_limits<T>::max());
    return dist(RNG());
}

template <>
uint8_t RandomValue<uint8_t>()
{
    std::uniform_int_distribution<uint16_t> dist(0, 255);  // can't be uint8_t :/
    return uint8_t(dist(RNG()));
}

template <typename T>
void MakeWhiteNoise(std::vector<T>& pixels, size_t width, size_t height)
{
    pixels.resize(width*height);

    // NOTE: this works, but won't give a balanced histogram
    //for (T& pixel : pixels)
        //pixel = RandomValue<T>();

    for (size_t index = 0, count = width * height; index < count; ++index)
    {
        float percent = float(index) / float(count - 1);
        float value = Lerp(0, float(std::numeric_limits<T>::max() + 1), percent); // intentionally not using convert.h conversion. this is subtly different.
        value = Clamp(0.0f, float(std::numeric_limits<T>::max()), value);
        pixels[index] = T(value);
    }

    std::shuffle(pixels.begin(), pixels.end(), RNG());
}