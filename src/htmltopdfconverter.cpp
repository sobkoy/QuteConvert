#include "quteconvert/htmltopdfconverter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPageLayout>
#include <QUrl>
#include <QVariant>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace quteconvert {

namespace {

constexpr auto kReadinessScript = R"JS(
(() => {
    for (const image of document.images) {
        image.loading = 'eager';
    }
    const imagesReady = Array.from(document.images).every(image => image.complete);
    const fontsReady = !document.fonts || document.fonts.status === 'loaded';
    return imagesReady && fontsReady;
})()
)JS";

}  // namespace

HtmlToPdfConverter::HtmlToPdfConverter(QObject* parent)
    : QObject(parent),
      profile_(new QWebEngineProfile(this)),
      page_(new QWebEnginePage(profile_, this)) {
  timeout_timer_.setSingleShot(true);
  readiness_timer_.setInterval(250);
  settle_timer_.setSingleShot(true);

  connect(page_, &QWebEnginePage::loadProgress, this,
          &HtmlToPdfConverter::LoadProgress);
  connect(page_, &QWebEnginePage::loadFinished, this,
          &HtmlToPdfConverter::HandleLoadFinished);
  connect(page_, &QWebEnginePage::pdfPrintingFinished, this,
          &HtmlToPdfConverter::HandlePdfPrintingFinished);
  connect(&readiness_timer_, &QTimer::timeout, this,
          &HtmlToPdfConverter::PollDocumentReadiness);
  connect(&settle_timer_, &QTimer::timeout, this,
          &HtmlToPdfConverter::BeginPrinting);
  connect(&timeout_timer_, &QTimer::timeout, this, [this] {
    const QString activity =
        state_ == State::kPrinting
            ? QStringLiteral("printing the PDF")
            : QStringLiteral("loading the document and its resources");
    Fail(QStringLiteral("Timed out while %1.").arg(activity));
  });
}

bool HtmlToPdfConverter::IsBusy() const { return state_ != State::kIdle; }

void HtmlToPdfConverter::Convert(const ConversionJob& job,
                                 const ConversionOptions& options) {
  if (IsBusy()) {
    emit Completed(job, false,
                   QStringLiteral("The converter is already busy."));
    return;
  }

  current_job_ = job;
  current_options_ = options;
  state_ = State::kLoading;

  auto* settings = page_->settings();
  settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  settings->setAttribute(QWebEngineSettings::AutoLoadImages, true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,
                         true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,
                         options.allow_remote_resources);
  settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);

  timeout_timer_.start(options.load_timeout_ms);
  page_->load(
      QUrl::fromLocalFile(QFileInfo(job.input_path).absoluteFilePath()));
}

void HtmlToPdfConverter::Cancel() {
  if (!IsBusy()) {
    return;
  }
  page_->triggerAction(QWebEnginePage::Stop);
  Finish(false, QStringLiteral("Conversion cancelled."));
}

void HtmlToPdfConverter::HandleLoadFinished(bool success) {
  if (state_ != State::kLoading) {
    return;
  }
  if (!success) {
    Fail(QStringLiteral("Qt WebEngine could not load the HTML document."));
    return;
  }

  state_ = State::kWaitingForResources;
  readiness_timer_.start();
  PollDocumentReadiness();
}

void HtmlToPdfConverter::PollDocumentReadiness() {
  if (state_ != State::kWaitingForResources) {
    return;
  }

  page_->runJavaScript(
      QString::fromUtf8(kReadinessScript), [this](const QVariant& result) {
        if (state_ != State::kWaitingForResources || !result.toBool()) {
          return;
        }
        readiness_timer_.stop();
        settle_timer_.start(current_options_.settle_delay_ms);
      });
}

void HtmlToPdfConverter::BeginPrinting() {
  if (state_ != State::kWaitingForResources) {
    return;
  }

  state_ = State::kPrinting;
  timeout_timer_.start(current_options_.load_timeout_ms);

  const QString temporary_path = TemporaryPdfPath();
  QFile::remove(temporary_path);
  const QPageLayout layout(
      current_options_.page_size, current_options_.orientation,
      current_options_.margins_mm, QPageLayout::Millimeter);
  page_->printToPdf(temporary_path, layout);
}

void HtmlToPdfConverter::HandlePdfPrintingFinished(const QString& file_path,
                                                   bool success) {
  if (state_ != State::kPrinting || file_path != TemporaryPdfPath()) {
    return;
  }
  if (!success) {
    Fail(QStringLiteral("Qt WebEngine failed to create the PDF."));
    return;
  }

  if (QFileInfo::exists(current_job_.output_path)) {
    if (current_options_.existing_file_policy !=
        ExistingFilePolicy::kOverwrite) {
      Fail(QStringLiteral(
          "The output file appeared while conversion was running."));
      return;
    }
    if (!QFile::remove(current_job_.output_path)) {
      Fail(QStringLiteral("Could not replace the existing output file."));
      return;
    }
  }

  if (!QFile::rename(file_path, current_job_.output_path)) {
    Fail(QStringLiteral(
        "Could not move the completed PDF to its final location."));
    return;
  }
  Finish(true);
}

void HtmlToPdfConverter::Fail(const QString& message) {
  page_->triggerAction(QWebEnginePage::Stop);
  QFile::remove(TemporaryPdfPath());
  Finish(false, message);
}

void HtmlToPdfConverter::Finish(bool success, const QString& message) {
  timeout_timer_.stop();
  readiness_timer_.stop();
  settle_timer_.stop();

  const ConversionJob finished_job = current_job_;
  state_ = State::kIdle;
  current_job_ = {};
  emit Completed(finished_job, success, message);
}

QString HtmlToPdfConverter::TemporaryPdfPath() const {
  const QFileInfo output_info(current_job_.output_path);
  return output_info.absoluteDir().filePath(
      QStringLiteral(".%1.quteconvert-part.pdf")
          .arg(output_info.completeBaseName()));
}

}  // namespace quteconvert
