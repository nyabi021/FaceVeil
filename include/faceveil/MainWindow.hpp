#pragma once

#include <QMainWindow>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QThread;

namespace faceveil
{
    class ProcessorWorker;

    class MainWindow final : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);

        ~MainWindow() override;

    protected:
        void dragEnterEvent(QDragEnterEvent *event) override;

        void dropEvent(QDropEvent *event) override;

    private slots:
        void chooseModel();

        void chooseFiles();

        void chooseFolder();

        void chooseOutputDirectory();

        void startProcessing();

        void stopProcessing() const;

        void onWorkerFinished(bool cancelled);

    private:
        void addInputPath(const QString &path) const;

        void populateBundledModels() const;

        void updateModelPathFromSelection() const;

        [[nodiscard]] QString selectedModelPath() const;

        void setProcessing(bool processing) const;

        [[nodiscard]] QStringList inputPaths() const;

        void appendLog(const QString &message) const;

        QComboBox *modelCombo_ = nullptr;
        QLineEdit *modelPathEdit_ = nullptr;
        QLineEdit *outputDirEdit_ = nullptr;
        QListWidget *inputList_ = nullptr;
        QCheckBox *recursiveCheck_ = nullptr;
        QDoubleSpinBox *scoreThresholdSpin_ = nullptr;
        QDoubleSpinBox *nmsThresholdSpin_ = nullptr;
        QSpinBox *blockSizeSpin_ = nullptr;
        QDoubleSpinBox *paddingSpin_ = nullptr;
        QProgressBar *progressBar_ = nullptr;
        QPlainTextEdit *logEdit_ = nullptr;
        QPushButton *startButton_ = nullptr;
        QPushButton *stopButton_ = nullptr;
        QLabel *statusLabel_ = nullptr;

        QThread *workerThread_ = nullptr;
        ProcessorWorker *worker_ = nullptr;
    };
} // namespace faceveil
