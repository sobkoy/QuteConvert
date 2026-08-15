#pragma once

#include <QList>
#include <QObject>

#include "quteconvert/conversiontypes.h"

namespace quteconvert {

class HtmlToPdfConverter;

class ConversionController final : public QObject {
  Q_OBJECT

 public:
  explicit ConversionController(QObject* parent = nullptr);

  bool IsRunning() const;
  void Start(const QStringList& input_files, const QString& output_directory,
             const ConversionOptions& options);
  void Cancel();

 signals:
  void BatchStarted(int total_files);
  void FileStarted(const QString& input_path, int current, int total);
  void FileLoadProgress(int percent);
  void FileFinished(const QString& input_path, const QString& output_path,
                    bool success, const QString& message);
  void OverallProgress(int completed, int total);
  void LogMessage(const QString& message);
  void BatchFinished(int succeeded, int failed, int skipped);

 private:
  void ProcessNext();
  void HandleConversionCompleted(const ConversionJob& job, bool success,
                                 const QString& error_message);
  void FinishBatch();

  HtmlToPdfConverter* converter_{nullptr};
  QList<ConversionJob> jobs_;
  ConversionOptions options_;
  int current_index_{0};
  int succeeded_{0};
  int failed_{0};
  int skipped_{0};
  bool running_{false};
  bool finishing_{false};
  bool cancel_requested_{false};
};

}  // namespace quteconvert
