#include "quteconvert/conversioncontroller.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QTimer>

#include "quteconvert/filediscovery.h"
#include "quteconvert/htmltopdfconverter.h"

namespace quteconvert {

ConversionController::ConversionController(QObject* parent) : QObject(parent) {}

bool ConversionController::IsRunning() const { return running_; }

void ConversionController::Start(const QStringList& input_files,
                                 const QString& output_directory,
                                 const ConversionOptions& options) {
  if (running_) {
    emit LogMessage(QStringLiteral("A conversion batch is already running."));
    return;
  }
  if (input_files.isEmpty()) {
    emit LogMessage(QStringLiteral("No HTML files were selected."));
    emit BatchFinished(0, 0, 0);
    return;
  }
  if (!QDir().mkpath(output_directory)) {
    emit LogMessage(QStringLiteral("Could not create output directory: %1")
                        .arg(output_directory));
    emit BatchFinished(0, input_files.size(), 0);
    return;
  }

  jobs_.clear();
  QSet<QString> reserved_output_paths;
  for (const QString& input_path : input_files) {
    QString output_path =
        QFileInfo(FileDiscovery::OutputPathFor(input_path, output_directory))
            .absoluteFilePath();

    if (options.existing_file_policy == ExistingFilePolicy::kAddSuffix ||
        reserved_output_paths.contains(output_path)) {
      output_path =
          FileDiscovery::UniqueOutputPath(output_path, reserved_output_paths);
    }
    reserved_output_paths.insert(output_path);
    jobs_.append({input_path, output_path});
  }

  options_ = options;
  current_index_ = 0;
  succeeded_ = 0;
  failed_ = 0;
  skipped_ = 0;
  finishing_ = false;
  cancel_requested_ = false;
  running_ = true;

  converter_ = new HtmlToPdfConverter(this);
  connect(converter_, &HtmlToPdfConverter::LoadProgress, this,
          &ConversionController::FileLoadProgress);
  connect(converter_, &HtmlToPdfConverter::Completed, this,
          &ConversionController::HandleConversionCompleted);

  emit BatchStarted(jobs_.size());
  emit OverallProgress(0, jobs_.size());
  QTimer::singleShot(0, this, &ConversionController::ProcessNext);
}

void ConversionController::Cancel() {
  if (!running_ || finishing_) {
    return;
  }
  cancel_requested_ = true;
  emit LogMessage(
      QStringLiteral("Cancelling after the active operation stops..."));
  if (converter_ && converter_->IsBusy()) {
    converter_->Cancel();
  } else {
    FinishBatch();
  }
}

void ConversionController::ProcessNext() {
  if (!running_ || finishing_) {
    return;
  }
  if (cancel_requested_ || current_index_ >= jobs_.size()) {
    FinishBatch();
    return;
  }

  const ConversionJob& job = jobs_.at(current_index_);
  if (options_.existing_file_policy == ExistingFilePolicy::kSkip &&
      QFileInfo::exists(job.output_path)) {
    ++skipped_;
    ++current_index_;
    const QString message =
        QStringLiteral("Skipped existing file: %1").arg(job.output_path);
    emit LogMessage(message);
    emit FileFinished(job.input_path, job.output_path, false, message);
    emit OverallProgress(current_index_, jobs_.size());
    QTimer::singleShot(0, this, &ConversionController::ProcessNext);
    return;
  }

  emit FileStarted(job.input_path, current_index_ + 1, jobs_.size());
  emit LogMessage(QStringLiteral("Converting %1").arg(job.input_path));
  converter_->Convert(job, options_);
}

void ConversionController::HandleConversionCompleted(
    const ConversionJob& job, bool success, const QString& error_message) {
  if (!running_ || finishing_) {
    return;
  }

  if (cancel_requested_) {
    emit LogMessage(QStringLiteral("Batch cancelled."));
    FinishBatch();
    return;
  }

  if (success) {
    ++succeeded_;
    emit LogMessage(QStringLiteral("Created %1").arg(job.output_path));
  } else {
    ++failed_;
    emit LogMessage(
        QStringLiteral("Failed %1: %2").arg(job.input_path, error_message));
  }
  emit FileFinished(job.input_path, job.output_path, success, error_message);

  ++current_index_;
  emit OverallProgress(current_index_, jobs_.size());
  QTimer::singleShot(0, this, &ConversionController::ProcessNext);
}

void ConversionController::FinishBatch() {
  if (!running_ || finishing_) {
    return;
  }
  finishing_ = true;
  jobs_.clear();

  const int succeeded = succeeded_;
  const int failed = failed_;
  const int skipped = skipped_;
  HtmlToPdfConverter* finished_converter = converter_;
  converter_ = nullptr;

  if (!finished_converter) {
    finishing_ = false;
    running_ = false;
    emit BatchFinished(succeeded, failed, skipped);
    return;
  }

  connect(
      finished_converter, &QObject::destroyed, this,
      [this, succeeded, failed, skipped] {
        finishing_ = false;
        running_ = false;
        emit BatchFinished(succeeded, failed, skipped);
      },
      Qt::SingleShotConnection);
  finished_converter->deleteLater();
}

}  // namespace quteconvert
