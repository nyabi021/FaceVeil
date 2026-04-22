#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

#include <atomic>

namespace faceveil
{
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
                        QObject *reviewReceiver);

    public slots:
        void process();

        void cancel();

    signals:
        void progressChanged(int completed, int total);

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
    };
} // namespace faceveil
