#include "faceveil/Mosaic.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>

namespace faceveil
{
    void applyMosaic(const cv::Mat &image, const FaceDetections &detections, int blockSize, float paddingRatio)
    {
        if (image.empty())
        {
            return;
        }

        const int width = image.cols;
        const int height = image.rows;

        blockSize = std::max(blockSize, 2);

        for (const auto &detection: detections)
        {
            auto box = detection.box;
            const float padX = box.width * paddingRatio;
            const float padY = box.height * paddingRatio;

            int x = static_cast<int>(std::floor(box.x - padX));
            int y = static_cast<int>(std::floor(box.y - padY));
            int right = static_cast<int>(std::ceil(box.x + box.width + padX));
            int bottom = static_cast<int>(std::ceil(box.y + box.height + padY));

            x = std::clamp(x, 0, width - 1);
            y = std::clamp(y, 0, height - 1);
            right = std::clamp(right, x + 1, width);
            bottom = std::clamp(bottom, y + 1, height);

            cv::Rect roiRect(x, y, right - x, bottom - y);
            cv::Mat roi = image(roiRect);

            const int smallWidth = std::clamp(roi.cols / blockSize, 1, std::max(1, roi.cols / 2));
            const int smallHeight = std::clamp(roi.rows / blockSize, 1, std::max(1, roi.rows / 2));

            cv::Mat small;
            cv::resize(roi, small, cv::Size(smallWidth, smallHeight), 0.0, 0.0, cv::INTER_LINEAR);
            cv::resize(small, roi, roi.size(), 0.0, 0.0, cv::INTER_NEAREST);
        }
    }
} // namespace faceveil
