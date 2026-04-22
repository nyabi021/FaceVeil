#pragma once

#include "faceveil/FaceDetection.hpp"

#include <opencv2/core.hpp>

namespace faceveil
{
    void applyMosaic(const cv::Mat &image, const FaceDetections &detections, int blockSize, float paddingRatio);
} // namespace faceveil
