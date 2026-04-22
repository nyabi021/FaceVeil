#pragma once

#include <opencv2/core.hpp>

#include <vector>

namespace faceveil
{
    struct FaceDetection
    {
        cv::Rect2f box;
        float score = 0.0F;
    };

    using FaceDetections = std::vector<FaceDetection>;
} // namespace faceveil
