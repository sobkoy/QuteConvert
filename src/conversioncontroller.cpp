#include "quteconvert/conversioncontroller.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QTimer>

#include "quteconvert/filediscovery.h"
#include "quteconvert/htmltopdfconverter.h"

namespace quteconvert {

ConversionController::ConversionController(QObject* parent)
    : QObject(parent), converter_(new HtmlToPdfConverter(this)) {
  connect(converter_, &HtmlToPdfConverter::loadProgress, this,
          &ConversionController::fileLoadProgress);
  connect(converter_, &HtmlToPdfConverter::completed, this,
          &ConversionController::handleConversionCompleted);
}

bool ConversionController::isRunning() const { return running_; }

void ConversionController::start(const QStringList& inputFiles,
                                 const QString& outputDirectory,
                                 const ConversionOptions& options) {
  if (running_) {
    emit logMessage(QStringLiteral("A conversion batch is already running."));
    return;
  }
  if (inputFiles.isEmpty()) {
    emit logMessage(QStringLiteral("No HTML files were selected."));
    emit batchFinished(0, 0, 0);
    return;
  }
  if (!QDir().mkpath(outputDirectory)) {
    emit logMessage(QStringLiteral("Could not create output directory: %1")
                        .arg(outputDirectory));
    emit batchFinished(0, inputFiles.size(), 0);
    return;
  }

  jobs_.clear();
  QSet<QString> reservedOutputPaths;
  for (const QString& inputPath : inputFiles) {
    QString outputPath =
        QFileInfo(FileDiscovery::outputPathFor(inputPath, outputDirectory))
            .absoluteFilePath();

    if (options.existingFilePolicy == ExistingFilePolicy::AddSuffix ||
        reservedOutputPaths.contains(outputPath)) {
      outputPath =
          FileDiscovery::uniqueOutputPath(outputPath, reservedOutputPaths);
    }
    reservedOutputPaths.insert(outputPath);
    jobs_.append({inputPath, outputPath});
  }

  options_ = options;
  currentIndex_ = 0;
  succeeded_ = 0;
  failed_ = 0;
  skipped_ = 0;
  cancelRequested_ = false;
  running_ = true;

  emit batchStarted(jobs_.size());
  emit overallProgress(0, jobs_.size());
  QTimer::singleShot(0, this, &ConversionController::processNext);
}

void ConversionController::cancel() {
  if (!running_) {
    return;
  }
  cancelRequested_ = true;
  emit logMessage(
      QStringLiteral("Cancelling after the active operation stops..."));
  if (converter_->isBusy()) {
    converter_->cancel();
  } else {
    finishBatch();
  }
}

void ConversionController::processNext() {
  if (!running_) {
    return;
  }
  if (cancelRequested_ || currentIndex_ >= jobs_.size()) {
    finishBatch();
    return;
  }

  const ConversionJob& job = jobs_.at(currentIndex_);
  if (options_.existingFilePolicy == ExistingFilePolicy::Skip &&
      QFileInfo::exists(job.outputPath)) {
    ++skipped_;
    ++currentIndex_;
    const QString message =
        QStringLiteral("Skipped existing file: %1").arg(job.outputPath);
    emit logMessage(message);
    emit fileFinished(job.inputPath, job.outputPath, false, message);
    emit overallProgress(currentIndex_, jobs_.size());
    QTimer::singleShot(0, this, &ConversionController::processNext);
    return;
  }

  emit fileStarted(job.inputPath, currentIndex_ + 1, jobs_.size());
  emit logMessage(QStringLiteral("Converting %1").arg(job.inputPath));
  converter_->convert(job, options_);
}

void ConversionController::handleConversionCompleted(
    const ConversionJob& job, bool success, const QString& errorMessage) {
  if (!running_) {
    return;
  }

  if (cancelRequested_) {
    emit logMessage(QStringLiteral("Batch cancelled."));
    finishBatch();
    return;
  }

  if (success) {
    ++succeeded_;
    emit logMessage(QStringLiteral("Created %1").arg(job.outputPath));
  } else {
    ++failed_;
    emit logMessage(
        QStringLiteral("Failed %1: %2").arg(job.inputPath, errorMessage));
  }
  emit fileFinished(job.inputPath, job.outputPath, success, errorMessage);

  ++currentIndex_;
  emit overallProgress(currentIndex_, jobs_.size());
  QTimer::singleShot(0, this, &ConversionController::processNext);
}

void ConversionController::finishBatch() {
  if (!running_) {
    return;
  }
  running_ = false;
  jobs_.clear();
  emit batchFinished(succeeded_, failed_, skipped_);
}

}  // namespace quteconvert
