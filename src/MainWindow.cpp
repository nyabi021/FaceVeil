#include "faceveil/MainWindow.hpp"

#include "faceveil/ProcessorWorker.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QWidget>

#include <array>
#include <memory>

namespace faceveil
{
    namespace
    {
        QString defaultOutputDirectory()
        {
            const auto pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
            if (!pictures.isEmpty())
            {
                return pictures + "/FaceVeil";
            }
            return QDir::homePath() + "/FaceVeil";
        }

        std::unique_ptr<QPushButton> smallButton(const QString &label)
        {
            auto button = std::make_unique<QPushButton>(label);
            button->setMinimumWidth(96);
            return button;
        }

        QString firstExistingModelPath(const QString &fileName)
        {
            const auto appDir = QCoreApplication::applicationDirPath();
            const std::array<QString, 4> candidates = {
                appDir + "/models/" + fileName,
                appDir + "/../Resources/models/" + fileName,
                appDir + "/../../../../models/" + fileName,
                QDir::currentPath() + "/models/" + fileName,
            };

            for (const auto &candidate: candidates)
            {
                const QFileInfo info(QDir::cleanPath(candidate));
                if (info.exists() && info.isFile())
                {
                    return info.absoluteFilePath();
                }
            }

            return {};
        }
    } // namespace

    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
    {
        setWindowTitle("FaceVeil");
        setAcceptDrops(true);
        resize(880, 680);

        auto central = std::make_unique<QWidget>(this);
        auto rootLayout = std::make_unique<QVBoxLayout>();
        auto *centralWidget = central.get();
        auto *root = rootLayout.get();
        root->setContentsMargins(18, 18, 18, 18);
        root->setSpacing(14);
        centralWidget->setLayout(rootLayout.release());

        modelCombo_ = new QComboBox(this);
        modelPathEdit_ = new QLineEdit(this);
        modelPathEdit_->setReadOnly(true);
        modelPathEdit_->setPlaceholderText("Bundled SCRFD model path");
        auto modelButton = smallButton("Model...");
        connect(modelButton.get(), &QPushButton::clicked, this, &MainWindow::chooseModel);
        connect(modelCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::updateModelPathFromSelection);

        auto modelChoiceRowLayout = std::make_unique<QHBoxLayout>();
        auto *modelChoiceRow = modelChoiceRowLayout.get();
        modelChoiceRow->addWidget(new QLabel("Face model", this));
        modelChoiceRow->addWidget(modelCombo_, 1);
        root->addLayout(modelChoiceRowLayout.release());

        auto modelRowLayout = std::make_unique<QHBoxLayout>();
        auto *modelRow = modelRowLayout.get();
        modelRow->addWidget(modelPathEdit_);
        modelRow->addWidget(modelButton.release());
        root->addLayout(modelRowLayout.release());

        inputList_ = new QListWidget(this);
        inputList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        inputList_->setMinimumHeight(150);
        inputList_->setAlternatingRowColors(true);
        root->addWidget(new QLabel("Inputs", this));
        root->addWidget(inputList_);

        auto inputButtonsLayout = std::make_unique<QHBoxLayout>();
        auto *inputButtons = inputButtonsLayout.get();
        auto addFilesButton = smallButton("Add Images...");
        auto addFolderButton = smallButton("Add Folder...");
        auto clearButton = smallButton("Clear");
        connect(addFilesButton.get(), &QPushButton::clicked, this, &MainWindow::chooseFiles);
        connect(addFolderButton.get(), &QPushButton::clicked, this, &MainWindow::chooseFolder);
        connect(clearButton.get(), &QPushButton::clicked, inputList_, &QListWidget::clear);
        inputButtons->addWidget(addFilesButton.release());
        inputButtons->addWidget(addFolderButton.release());
        inputButtons->addWidget(clearButton.release());
        inputButtons->addStretch();
        root->addLayout(inputButtonsLayout.release());

        auto outputRowLayout = std::make_unique<QHBoxLayout>();
        auto *outputRow = outputRowLayout.get();
        outputDirEdit_ = new QLineEdit(defaultOutputDirectory(), this);
        auto outputButton = smallButton("Output...");
        connect(outputButton.get(), &QPushButton::clicked, this, &MainWindow::chooseOutputDirectory);
        outputRow->addWidget(outputDirEdit_);
        outputRow->addWidget(outputButton.release());
        root->addLayout(outputRowLayout.release());

        auto optionsLayout = std::make_unique<QFormLayout>();
        auto *options = optionsLayout.get();
        recursiveCheck_ = new QCheckBox("Include subfolders", this);
        recursiveCheck_->setChecked(true);

        scoreThresholdSpin_ = new QDoubleSpinBox(this);
        scoreThresholdSpin_->setRange(0.05, 0.99);
        scoreThresholdSpin_->setSingleStep(0.05);
        scoreThresholdSpin_->setValue(0.5);

        nmsThresholdSpin_ = new QDoubleSpinBox(this);
        nmsThresholdSpin_->setRange(0.05, 0.95);
        nmsThresholdSpin_->setSingleStep(0.05);
        nmsThresholdSpin_->setValue(0.4);

        blockSizeSpin_ = new QSpinBox(this);
        blockSizeSpin_->setRange(2, 200);
        blockSizeSpin_->setValue(28);

        paddingSpin_ = new QDoubleSpinBox(this);
        paddingSpin_->setRange(0.0, 1.0);
        paddingSpin_->setSingleStep(0.05);
        paddingSpin_->setValue(0.18);

        options->addRow("", recursiveCheck_);
        options->addRow("Score threshold", scoreThresholdSpin_);
        options->addRow("NMS threshold", nmsThresholdSpin_);
        options->addRow("Mosaic block size", blockSizeSpin_);
        options->addRow("Face padding", paddingSpin_);
        root->addLayout(optionsLayout.release());

        progressBar_ = new QProgressBar(this);
        progressBar_->setRange(0, 100);
        progressBar_->setValue(0);
        root->addWidget(progressBar_);

        logEdit_ = new QPlainTextEdit(this);
        logEdit_->setReadOnly(true);
        logEdit_->setMinimumHeight(150);
        root->addWidget(logEdit_);

        auto actionRowLayout = std::make_unique<QHBoxLayout>();
        auto *actionRow = actionRowLayout.get();
        actionRow->addStretch();
        stopButton_ = new QPushButton("Stop", this);
        startButton_ = new QPushButton("Start", this);
        startButton_->setDefault(true);
        stopButton_->setEnabled(false);
        connect(startButton_, &QPushButton::clicked, this, &MainWindow::startProcessing);
        connect(stopButton_, &QPushButton::clicked, this, &MainWindow::stopProcessing);
        actionRow->addWidget(stopButton_);
        actionRow->addWidget(startButton_);
        root->addLayout(actionRowLayout.release());

        setCentralWidget(central.release());
        populateBundledModels();
        appendLog("Drop images or folders here, then choose a model speed/quality mode.");
    }

    MainWindow::~MainWindow()
    {
        if (workerThread_ != nullptr)
        {
            stopProcessing();
            workerThread_->quit();
            workerThread_->wait();
        }
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent *event)
    {
        if (event->mimeData()->hasUrls())
        {
            event->acceptProposedAction();
        }
    }

    void MainWindow::dropEvent(QDropEvent *event)
    {
        for (const auto &url: event->mimeData()->urls())
        {
            if (url.isLocalFile())
            {
                addInputPath(url.toLocalFile());
            }
        }
        event->acceptProposedAction();
    }

    void MainWindow::chooseModel()
    {
        const auto path = QFileDialog::getOpenFileName(this, "Select SCRFD ONNX Model", QDir::currentPath(),
                                                       "ONNX Models (*.onnx)");
        if (!path.isEmpty())
        {
            const QFileInfo info(path);
            modelCombo_->addItem("Custom - " + info.fileName(), info.absoluteFilePath());
            modelCombo_->setCurrentIndex(modelCombo_->count() - 1);
            modelPathEdit_->setText(path);
        }
    }

    void MainWindow::chooseFiles()
    {
        const auto files = QFileDialog::getOpenFileNames(this,
                                                         "Select Images",
                                                         QStandardPaths::writableLocation(
                                                             QStandardPaths::PicturesLocation),
                                                         "Images (*.jpg *.jpeg *.png *.bmp *.tif *.tiff *.webp)");
        for (const auto &file: files)
        {
            addInputPath(file);
        }
    }

    void MainWindow::chooseFolder()
    {
        const auto folder = QFileDialog::getExistingDirectory(this, "Select Folder",
                                                              QStandardPaths::writableLocation(
                                                                  QStandardPaths::PicturesLocation));
        if (!folder.isEmpty())
        {
            addInputPath(folder);
        }
    }

    void MainWindow::chooseOutputDirectory()
    {
        const auto folder = QFileDialog::getExistingDirectory(this, "Select Output Folder", outputDirEdit_->text());
        if (!folder.isEmpty())
        {
            outputDirEdit_->setText(folder);
        }
    }

    void MainWindow::startProcessing()
    {
        const auto modelPath = selectedModelPath();
        if (modelPath.isEmpty() || !QFileInfo::exists(modelPath))
        {
            appendLog("Choose a valid SCRFD ONNX model first.");
            return;
        }

        if (inputList_->count() == 0)
        {
            appendLog("Add at least one image or folder.");
            return;
        }

        if (outputDirEdit_->text().isEmpty())
        {
            appendLog("Choose an output folder.");
            return;
        }

        setProcessing(true);
        progressBar_->setValue(0);

        workerThread_ = new QThread(this);
        worker_ = new ProcessorWorker(modelPath,
                                      inputPaths(),
                                      outputDirEdit_->text(),
                                      recursiveCheck_->isChecked(),
                                      static_cast<float>(scoreThresholdSpin_->value()),
                                      static_cast<float>(nmsThresholdSpin_->value()),
                                      blockSizeSpin_->value(),
                                      static_cast<float>(paddingSpin_->value()));

        worker_->moveToThread(workerThread_);
        connect(workerThread_, &QThread::started, worker_, &ProcessorWorker::process);
        connect(worker_, &ProcessorWorker::logMessage, this, &MainWindow::appendLog);
        connect(worker_, &ProcessorWorker::progressChanged, this, [this](int completed, int total)
        {
            progressBar_->setRange(0, std::max(total, 1));
            progressBar_->setValue(completed);
        });
        connect(worker_, &ProcessorWorker::finished, this, &MainWindow::onWorkerFinished);
        connect(worker_, &ProcessorWorker::finished, workerThread_, &QThread::quit);
        connect(workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
        connect(workerThread_, &QThread::finished, workerThread_, &QObject::deleteLater);

        appendLog("Starting...");
        workerThread_->start();
    }

    void MainWindow::stopProcessing() const
    {
        if (worker_ != nullptr)
        {
            appendLog("Stopping after the current image...");
            QMetaObject::invokeMethod(worker_, "cancel", Qt::QueuedConnection);
        }
    }

    void MainWindow::onWorkerFinished(bool cancelled)
    {
        appendLog(cancelled ? "Cancelled." : "Finished.");
        setProcessing(false);
        worker_ = nullptr;
        workerThread_ = nullptr;
    }

    void MainWindow::addInputPath(const QString &path) const
    {
        if (path.isEmpty())
        {
            return;
        }

        for (int i = 0; i < inputList_->count(); ++i)
        {
            if (inputList_->item(i)->text() == path)
            {
                return;
            }
        }
        inputList_->addItem(path);
    }

    void MainWindow::setProcessing(bool processing) const
    {
        startButton_->setEnabled(!processing);
        stopButton_->setEnabled(processing);
        modelCombo_->setEnabled(!processing);
        modelPathEdit_->setEnabled(!processing);
        outputDirEdit_->setEnabled(!processing);
        inputList_->setEnabled(!processing);
        recursiveCheck_->setEnabled(!processing);
        scoreThresholdSpin_->setEnabled(!processing);
        nmsThresholdSpin_->setEnabled(!processing);
        blockSizeSpin_->setEnabled(!processing);
        paddingSpin_->setEnabled(!processing);
    }

    QStringList MainWindow::inputPaths() const
    {
        QStringList paths;
        for (int i = 0; i < inputList_->count(); ++i)
        {
            paths.append(inputList_->item(i)->text());
        }
        return paths;
    }

    void MainWindow::populateBundledModels() const
    {
        struct ModelOption
        {
            QString label;
            QString fileName;
        };

        const std::array<ModelOption, 2> options = {
            ModelOption{"Fast - SCRFD 2.5G", "2.5g_bnkps.onnx"},
            ModelOption{"Accurate - SCRFD 10G", "10g_bnkps.onnx"},
        };

        for (const auto &option: options)
        {
            const auto path = firstExistingModelPath(option.fileName);
            modelCombo_->addItem(option.label, path);
        }

        modelCombo_->setCurrentIndex(0);
        updateModelPathFromSelection();

        if (selectedModelPath().isEmpty())
        {
            appendLog("Bundled models were not found. Use Model... to select an ONNX file.");
        }
    }

    void MainWindow::updateModelPathFromSelection() const
    {
        modelPathEdit_->setText(selectedModelPath());
    }

    QString MainWindow::selectedModelPath() const
    {
        if (modelCombo_ == nullptr || modelCombo_->currentIndex() < 0)
        {
            return {};
        }

        return modelCombo_->currentData().toString();
    }

    void MainWindow::appendLog(const QString &message) const
    {
        const auto time = QDateTime::currentDateTime().toString("HH:mm:ss");
        logEdit_->appendPlainText(QString("[%1] %2").arg(time, message));
    }
} // namespace faceveil
