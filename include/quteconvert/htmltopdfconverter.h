#pragma once

#include <QObject>
#include <QTimer>

#include "quteconvert/conversiontypes.h"

class QWebEnginePage;
class QWebEngineProfile;

namespace quteconvert {

class HtmlToPdfConverter final : public QObject {
  Q_OBJECT

 public:
  explicit HtmlToPdfConverter(QObject* parent = nullptr);

  bool isBusy() const;
  void convert(const ConversionJob& job, const ConversionOptions& options);
  void cancel();

 signals:
  void loadProgress(int percent);
  void completed(const quteconvert::ConversionJob& job, bool success,
                 const QString& errorMessage);

 private:
  enum class State { Idle, Loading, WaitingForResources, Printing };

  void handleLoadFinished(bool success);
  void pollDocumentReadiness();
  void beginPrinting();
  void handlePdfPrintingFinished(const QString& filePath, bool success);
  void fail(const QString& message);
  void finish(bool success, const QString& message = {});
  QString temporaryPdfPath() const;

  QWebEngineProfile* profile_{nullptr};
  QWebEnginePage* page_{nullptr};
  QTimer timeoutTimer_;
  QTimer readinessTimer_;
  QTimer settleTimer_;
  ConversionJob currentJob_;
  ConversionOptions currentOptions_;
  State state_{State::Idle};
};

}  // namespace quteconvert
