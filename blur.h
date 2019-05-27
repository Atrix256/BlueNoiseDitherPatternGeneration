#pragma once

#include <vector>

void GaussianBlur(const std::vector<float>& srcImage, std::vector<float> &destImage, size_t width, float blurSigma);
