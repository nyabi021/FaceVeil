#include "faceveil/ProcessorWorker.hpp"

#include "faceveil/ImageScanner.hpp"
#include "faceveil/Mosaic.hpp"
#include "faceveil/ScrfdFaceDetector.hpp"

#include <QDir>

#include <opencv2/imgcodecs.hpp>

#include <exception>
#include <filesystem>

namespace faceveil
{
    ProcessorWorker::ProcessorWorker(QString modelPath,
                                     QStringList inputs,
                                     QString outputDirectory,
                                     bool recursive,
                                     float scoreThreshold,
                                     float nmsThreshold,
                                     int mosaicBlockSize,
                                     float paddingRatio)
        : modelPath_(std::move(modelPath)),
          inputs_(std::move(inputs)),
          outputDirectory_(std::move(outputDirectory)),
          recursive_(recursive),
          scoreThreshold_(scoreThreshold),
          nmsThreshold_(nmsThreshold),
          mosaicBlockSize_(mosaicBlockSize),
          paddingRatio_(paddingRatio)
    {
    }

    void ProcessorWorker::process()
    {
        cancelled_ = false;

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

            int completed = 0;
            for (const auto &item: images)
            {
                if (cancelled_)
                {
                    emit finished(true);
                    return;
                }

                const auto source = item.sourcePath;
                const auto destination = outputRoot / item.relativePath;
                std::filesystem::create_directories(destination.parent_path());

                cv::Mat image = cv::imread(source.string(), cv::IMREAD_COLOR);
                if (image.empty())
                {
                    emit logMessage(
                        QString("Skipped unreadable image: %1").arg(QString::fromStdString(source.string())));
                    emit progressChanged(++completed, total);
                    continue;
                }

                const auto faces = detector.detect(image, scoreThreshold_, nmsThreshold_);
                applyMosaic(image, faces, mosaicBlockSize_, paddingRatio_);

                if (!cv::imwrite(destination.string(), image))
                {
                    emit logMessage(QString("Failed to save: %1").arg(QString::fromStdString(destination.string())));
                } else
                {
                    emit logMessage(QString("Processed %1 face(s): %2")
                        .arg(static_cast<int>(faces.size()))
                        .arg(QString::fromStdString(source.filename().string())));
                }

                emit progressChanged(++completed, total);
            }

            emit logMessage("Done.");
            emit finished(false);
        } catch (const std::exception &exception)
        {
            emit logMessage(QString("Error: %1").arg(exception.what()));
            emit finished(cancelled_);
        }
    }

    void ProcessorWorker::cancel()
    {
        cancelled_ = true;
    }
} // namespace faceveil
