#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

#include <atomic>
#include <memory>

namespace faceveil
{
    class ScrfdFaceDetector;

    class ProcessorWorker final : public QObject
    {
        Q_OBJECT

    public:
        ProcessorWorker(QString modelPath,
                        QStringList inputs,
                        QString outputDirectory,
                        bool recursive,
                        float scoreThreshold,
                        float nmsThreshold,
                        int mosaicBlockSize,
                        float paddingRatio,
                        bool reviewEnabled,
                        QObject *reviewReceiver,
                        std::shared_ptr<ScrfdFaceDetector> cachedDetector = {});

        ~ProcessorWorker() override;

        [[nodiscard]] std::shared_ptr<ScrfdFaceDetector> takeDetector();

    public slots:
        void process();

        void cancel();

    signals:
        void progressChanged(int completed, int total);

        void stageChanged(int index, int total, const QString &stage, const QString &fileName);

        void logMessage(const QString &message);

        void finished(bool cancelled);

    private:
        QString modelPath_;
        QStringList inputs_;
        QString outputDirectory_;
        bool recursive_;
        float scoreThreshold_;
        float nmsThreshold_;
        int mosaicBlockSize_;
        float paddingRatio_;
        bool reviewEnabled_;
        QPointer<QObject> reviewReceiver_;
        std::atomic<bool> cancelled_{false};
        std::shared_ptr<ScrfdFaceDetector> detector_;
    };
}
