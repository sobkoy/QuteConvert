#pragma once

#include "quteconvert/conversiontypes.h"

#include <QList>
#include <QObject>

namespace quteconvert {

class HtmlToPdfConverter;

class ConversionController final : public QObject {
    Q_OBJECT

public:
    explicit ConversionController(QObject *parent = nullptr);

    bool isRunning() const;
    void start(const QStringList &inputFiles,
               const QString &outputDirectory,
               const ConversionOptions &options);
    void cancel();

signals:
    void batchStarted(int totalFiles);
    void fileStarted(const QString &inputPath, int current, int total);
    void fileLoadProgress(int percent);
    void fileFinished(const QString &inputPath,
                      const QString &outputPath,
                      bool success,
                      const QString &message);
    void overallProgress(int completed, int total);
    void logMessage(const QString &message);
    void batchFinished(int succeeded, int failed, int skipped);

private:
    void processNext();
    void handleConversionCompleted(const ConversionJob &job,
                                   bool success,
                                   const QString &errorMessage);
    void finishBatch();

    HtmlToPdfConverter *converter_{nullptr};
    QList<ConversionJob> jobs_;
    ConversionOptions options_;
    int currentIndex_{0};
    int succeeded_{0};
    int failed_{0};
    int skipped_{0};
    bool running_{false};
    bool cancelRequested_{false};
};

} // namespace quteconvert
