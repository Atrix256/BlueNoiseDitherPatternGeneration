#pragma once

#include "misc.h"

template <typename T>
float ToFloat(T value)
{
    return float(value) / float(std::numeric_limits<T>::max());
}

template <typename T>
T FromFloat(float value)
{
    value = Lerp(0, float(std::numeric_limits<T>::max() + 1), value);
    value = Clamp(0.0f, float(std::numeric_limits<T>::max()), value);
    return T(value);
}