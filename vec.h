#pragma once

#include <array>

typedef unsigned int uint;

typedef std::array<float, 2> vec2;
typedef std::array<float, 3> vec3;
typedef std::array<float, 4> vec4;
typedef std::array<int, 2> ivec2;
typedef std::array<uint, 2> uvec2;

template <size_t N>
inline std::array<float, N> operator* (const std::array<float, N>& A, const std::array<float, N>& B)
{
    std::array<float, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] * B[i];
    return ret;
}

template <size_t N>
inline std::array<float, N> operator/ (const std::array<float, N>& A, const std::array<float, N>& B)
{
    std::array<float, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] / B[i];
    return ret;
}

template <typename T, size_t N>
inline std::array<T, N> operator+ (const std::array<T, N>& A, const std::array<T, N>& B)
{
    std::array<T, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] + B[i];
    return ret;
}

template <typename T, size_t N>
inline std::array<T, N> operator- (const std::array<T, N>& A, const std::array<T, N>& B)
{
    std::array<T, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] - B[i];
    return ret;
}

template <size_t N>
inline std::array<int, N> operator^ (const std::array<int, N>& A, const std::array<int, N>& B)
{
    std::array<int, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] ^ B[i];
    return ret;
}

template <size_t N>
inline std::array<int, N> operator% (const std::array<int, N>& A, const std::array<int, N>& B)
{
    std::array<int, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] % B[i];
    return ret;
}

template <size_t N>
inline std::array<float, N> operator+ (const std::array<float, N>& A, float B)
{
    std::array<float, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] + B;
    return ret;
}

template <size_t N>
inline std::array<float, N> operator* (const std::array<float, N>& A, float B)
{
    std::array<float, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] * B;
    return ret;
}

template <size_t N>
inline std::array<float, N> fract(const std::array<float, N>& A)
{
    std::array<float, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = std::fmodf(A[i], 1.0f);
    return ret;
}

template <size_t N>
inline std::array<uint, N> operator& (const std::array<uint, N>& A, const std::array<uint, N>& B)
{
    std::array<uint, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] & B[i];
    return ret;
}

template <size_t N>
inline std::array<uint, N> greaterThan (const std::array<uint, N>& A, const std::array<uint, N>& B)
{
    std::array<uint, N> ret;
    for (size_t i = 0; i < N; ++i)
        ret[i] = A[i] > B[i];
    return ret;
}

inline float fract(float A)
{
    return std::fmodf(A, 1.0f);
}

inline float max(float A, float B)
{
    return std::max(A, B);
}

template <size_t N>
inline float dot(const std::array<float, N>& A, const std::array<float, N>& B)
{
    float ret = 0.0f;
    for (size_t i = 0; i < N; ++i)
        ret += A[i] * B[i];
    return ret;
}

template <size_t N>
inline float length(const std::array<float, N>& A)
{
    float lengthSq = 0.0f;
    for (size_t i = 0; i < N; ++i)
        lengthSq += A[i] * A[i];
    return std::sqrtf(lengthSq);
}

inline float clamp(float value, float min, float max)
{
    if (value <= min)
        return min;
    else if (value >= max)
        return max;
    else
        return value;
}

inline std::array<float, 3> yzx(const std::array<float, 3>& A)
{
    return { A[1], A[2], A[0] };
}

inline std::array<float, 2> xx(const std::array<float, 3>& A)
{
    return { A[0], A[0] };
}

inline std::array<float, 2> xy(const std::array<float, 3>& A)
{
    return { A[0], A[1] };
}

inline std::array<float, 2> yz(const std::array<float, 3>& A)
{
    return { A[1], A[2] };
}

inline std::array<float, 2> zy(const std::array<float, 3>& A)
{
    return { A[2], A[1] };
}

inline float step(float edge, float x)
{
    return x >= edge;
}