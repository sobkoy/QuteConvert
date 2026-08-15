#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>

#include "quteconvert/conversiontypes.h"

class QWebEnginePage;
class QWebEngineProfile;

namespace quteconvert {

class HtmlToPdfConverter final : public QObject {
  Q_OBJECT

 public:
  explicit HtmlToPdfConverter(QObject* parent = nullptr);
  ~HtmlToPdfConverter() override;

  bool IsBusy() const;
  void Convert(const ConversionJob& job, const ConversionOptions& options);
  void Cancel();

 signals:
  void LoadProgress(int percent);
  void Completed(const quteconvert::ConversionJob& job, bool success,
                 const QString& error_message);

 private:
  enum class State {
    kIdle,
    kLoading,
    kWaitingForResources,
    kPrinting,
    kCleaningUp
  };

  void CreatePage();
  void DeletePage();
  void HandleLoadFinished(bool success);
  void PollDocumentReadiness();
  void BeginPrinting();
  void HandlePdfPrintingFinished(const QString& file_path, bool success);
  void Fail(const QString& message);
  void Finish(bool success, const QString& message = {});
  QString TemporaryPdfPath() const;

  QWebEngineProfile* profile_{nullptr};
  QPointer<QWebEnginePage> page_;
  QTimer timeout_timer_;
  QTimer readiness_timer_;
  QTimer settle_timer_;
  QTimer cleanup_timer_;
  ConversionJob current_job_;
  ConversionOptions current_options_;
  State state_{State::kIdle};
};

}  // namespace quteconvert
