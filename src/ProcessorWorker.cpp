#include "faceveil/ProcessorWorker.hpp"

#include "faceveil/ImageScanner.hpp"
#include "faceveil/Mosaic.hpp"
#include "faceveil/ReviewTypes.hpp"
#include "faceveil/ScrfdFaceDetector.hpp"

#include <QDir>
#include <QImage>
#include <QMetaObject>
#include <QRectF>
#include <QVector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <exception>
#include <filesystem>

namespace faceveil
{
    namespace
    {
        QImage matToQImage(const cv::Mat &bgr)
        {
            cv::Mat rgb;
            cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
            QImage image(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);
            return image.copy();
        }

        FaceDetections toDetections(const QVector<QRectF> &boxes)
        {
            FaceDetections result;
            result.reserve(boxes.size());
            for (const auto &rect: boxes)
            {
                FaceDetection det;
                det.box = cv::Rect2f(static_cast<float>(rect.x()),
                                     static_cast<float>(rect.y()),
                                     static_cast<float>(rect.width()),
                                     static_cast<float>(rect.height()));
                det.score = 1.0F;
                result.push_back(det);
            }
            return result;
        }

        QVector<QRectF> toQRects(const FaceDetections &faces)
        {
            QVector<QRectF> result;
            result.reserve(static_cast<int>(faces.size()));
            for (const auto &face: faces)
            {
                result.push_back(QRectF(face.box.x, face.box.y, face.box.width, face.box.height));
            }
            return result;
        }
    } // namespace

    ProcessorWorker::ProcessorWorker(QString modelPath,
                                     QStringList inputs,
                                     QString outputDirectory,
                                     bool recursive,
                                     float scoreThreshold,
                                     float nmsThreshold,
                                     int mosaicBlockSize,
                                     float paddingRatio,
                                     bool reviewEnabled,
                                     QObject *reviewReceiver)
        : modelPath_(std::move(modelPath)),
          inputs_(std::move(inputs)),
          outputDirectory_(std::move(outputDirectory)),
          recursive_(recursive),
          scoreThreshold_(scoreThreshold),
          nmsThreshold_(nmsThreshold),
          mosaicBlockSize_(mosaicBlockSize),
          paddingRatio_(paddingRatio),
          reviewEnabled_(reviewEnabled),
          reviewReceiver_(reviewReceiver)
    {
    }

    void ProcessorWorker::process()
    {
        cancelled_.store(false, std::memory_order_relaxed);

        try
        {
            emit logMessage("Loading SCRFD model...");
            ScrfdFaceDetector detector(modelPath_.toStdString());

            emit logMessage("Scanning images...");
            const auto images = scanImages(inputs_, recursive_);
            const int total = static_cast<int>(images.size());
            emit progressChanged(0, total);

            if (total == 0)
            {
                emit logMessage("No supported images were found.");
                emit finished(false);
                return;
            }

            const auto outputRoot = std::filesystem::path(outputDirectory_.toStdString());
            std::filesystem::create_directories(outputRoot);

            std::error_code canonicalError;
            const auto canonicalRoot = std::filesystem::weakly_canonical(outputRoot, canonicalError);
            const auto safeRoot = canonicalError ? outputRoot : canonicalRoot;

            int completed = 0;
            int index = 0;
            for (const auto &item: images)
            {
                ++index;
                if (cancelled_.load(std::memory_order_acquire))
                {
                    emit finished(true);
                    return;
                }

                const auto source = item.sourcePath;
                const auto destination = safeRoot / item.relativePath;

                std::error_code destError;
                const auto canonicalDestination = std::filesystem::weakly_canonical(destination, destError);
                const auto relativeFromRoot = destError
                    ? std::filesystem::path{}
                    : std::filesystem::relative(canonicalDestination, safeRoot, destError);
                const bool escaped = destError || relativeFromRoot.empty() ||
                    relativeFromRoot.begin()->string() == "..";
                if (escaped)
                {
                    emit logMessage(
                        QString("Skipped unsafe output path for: %1").arg(
                            QString::fromStdString(source.filename().string())));
                    emit progressChanged(++completed, total);
                    continue;
                }

                std::filesystem::create_directories(destination.parent_path());

                cv::Mat image = cv::imread(source.string(), cv::IMREAD_COLOR);
                if (image.empty())
                {
                    emit logMessage(
                        QString("Skipped unreadable image: %1").arg(QString::fromStdString(source.string())));
                    emit progressChanged(++completed, total);
                    continue;
                }

                const auto detected = detector.detect(image, scoreThreshold_, nmsThreshold_);
                FaceDetections finalFaces = detected;
                bool skipThisImage = false;

                if (reviewEnabled_ && reviewReceiver_)
                {
                    ReviewResult reviewResult;
                    const QImage preview = matToQImage(image);
                    const QVector<QRectF> detectedRects = toQRects(detected);
                    const QString sourceName = QString::fromStdString(source.filename().string());

                    const bool invoked = QMetaObject::invokeMethod(
                        reviewReceiver_.data(),
                        "requestReview",
                        Qt::BlockingQueuedConnection,
                        Q_RETURN_ARG(faceveil::ReviewResult, reviewResult),
                        Q_ARG(QImage, preview),
                        Q_ARG(QString, sourceName),
                        Q_ARG(QVector<QRectF>, detectedRects),
                        Q_ARG(int, index),
                        Q_ARG(int, total));

                    if (!invoked)
                    {
                        emit logMessage("Review bridge unavailable; saved without review.");
                    }
                    else
                    {
                        switch (reviewResult.decision)
                        {
                            case ReviewDecision::CancelAll:
                                cancelled_.store(true, std::memory_order_release);
                                emit finished(true);
                                return;
                            case ReviewDecision::Skip:
                                skipThisImage = true;
                                break;
                            case ReviewDecision::Save:
                                finalFaces = toDetections(reviewResult.finalBoxes);
                                break;
                        }
                    }
                }

                if (skipThisImage)
                {
                    if (!cv::imwrite(destination.string(), image))
                    {
                        emit logMessage(QString("Failed to copy: %1").arg(
                            QString::fromStdString(destination.string())));
                    }
                    else
                    {
                        emit logMessage(QString("Skipped (original copied): %1").arg(
                            QString::fromStdString(source.filename().string())));
                    }
                    emit progressChanged(++completed, total);
                    continue;
                }

                applyMosaic(image, finalFaces, mosaicBlockSize_, paddingRatio_);

                if (!cv::imwrite(destination.string(), image))
                {
                    emit logMessage(QString("Failed to save: %1").arg(QString::fromStdString(destination.string())));
                }
                else
                {
                    emit logMessage(QString("Processed %1 face(s): %2")
                        .arg(static_cast<int>(finalFaces.size()))
                        .arg(QString::fromStdString(source.filename().string())));
                }

                emit progressChanged(++completed, total);
            }

            emit logMessage("Done.");
            emit finished(false);
        } catch (const std::exception &exception)
        {
            emit logMessage(QString("Error: %1").arg(exception.what()));
            emit finished(cancelled_.load(std::memory_order_acquire));
        }
    }

    void ProcessorWorker::cancel()
    {
        cancelled_.store(true, std::memory_order_release);
    }
} // namespace faceveil
