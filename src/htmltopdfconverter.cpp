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

constexpr auto readinessScript = R"JS(
(() => {
    for (const image of document.images) {
        image.loading = 'eager';
    }
    const imagesReady = Array.from(document.images).every(image => image.complete);
    const fontsReady = !document.fonts || document.fonts.status === 'loaded';
    return imagesReady && fontsReady;
})()
)JS";

} // namespace

HtmlToPdfConverter::HtmlToPdfConverter(QObject *parent)
    : QObject(parent),
      profile_(new QWebEngineProfile(this)),
      page_(new QWebEnginePage(profile_, this)) {
    timeoutTimer_.setSingleShot(true);
    readinessTimer_.setInterval(250);
    settleTimer_.setSingleShot(true);

    connect(page_, &QWebEnginePage::loadProgress,
            this, &HtmlToPdfConverter::loadProgress);
    connect(page_, &QWebEnginePage::loadFinished,
            this, &HtmlToPdfConverter::handleLoadFinished);
    connect(page_, &QWebEnginePage::pdfPrintingFinished,
            this, &HtmlToPdfConverter::handlePdfPrintingFinished);
    connect(&readinessTimer_, &QTimer::timeout,
            this, &HtmlToPdfConverter::pollDocumentReadiness);
    connect(&settleTimer_, &QTimer::timeout,
            this, &HtmlToPdfConverter::beginPrinting);
    connect(&timeoutTimer_, &QTimer::timeout, this, [this] {
        const QString activity = state_ == State::Printing
                                     ? QStringLiteral("printing the PDF")
                                     : QStringLiteral("loading the document and its resources");
        fail(QStringLiteral("Timed out while %1.").arg(activity));
    });
}

bool HtmlToPdfConverter::isBusy() const {
    return state_ != State::Idle;
}

void HtmlToPdfConverter::convert(const ConversionJob &job,
                                 const ConversionOptions &options) {
    if (isBusy()) {
        emit completed(job, false, QStringLiteral("The converter is already busy."));
        return;
    }

    currentJob_ = job;
    currentOptions_ = options;
    state_ = State::Loading;

    auto *settings = page_->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,
                           options.allowRemoteResources);
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);

    timeoutTimer_.start(options.loadTimeoutMs);
    page_->load(QUrl::fromLocalFile(QFileInfo(job.inputPath).absoluteFilePath()));
}

void HtmlToPdfConverter::cancel() {
    if (!isBusy()) {
        return;
    }
    page_->triggerAction(QWebEnginePage::Stop);
    finish(false, QStringLiteral("Conversion cancelled."));
}

void HtmlToPdfConverter::handleLoadFinished(bool success) {
    if (state_ != State::Loading) {
        return;
    }
    if (!success) {
        fail(QStringLiteral("Qt WebEngine could not load the HTML document."));
        return;
    }

    state_ = State::WaitingForResources;
    readinessTimer_.start();
    pollDocumentReadiness();
}

void HtmlToPdfConverter::pollDocumentReadiness() {
    if (state_ != State::WaitingForResources) {
        return;
    }

    page_->runJavaScript(QString::fromUtf8(readinessScript), [this](const QVariant &result) {
        if (state_ != State::WaitingForResources || !result.toBool()) {
            return;
        }
        readinessTimer_.stop();
        settleTimer_.start(currentOptions_.settleDelayMs);
    });
}

void HtmlToPdfConverter::beginPrinting() {
    if (state_ != State::WaitingForResources) {
        return;
    }

    state_ = State::Printing;
    timeoutTimer_.start(currentOptions_.loadTimeoutMs);

    const QString temporaryPath = temporaryPdfPath();
    QFile::remove(temporaryPath);
    const QPageLayout layout(currentOptions_.pageSize,
                             currentOptions_.orientation,
                             currentOptions_.marginsMm,
                             QPageLayout::Millimeter);
    page_->printToPdf(temporaryPath, layout);
}

void HtmlToPdfConverter::handlePdfPrintingFinished(const QString &filePath,
                                                    bool success) {
    if (state_ != State::Printing || filePath != temporaryPdfPath()) {
        return;
    }
    if (!success) {
        fail(QStringLiteral("Qt WebEngine failed to create the PDF."));
        return;
    }

    if (QFileInfo::exists(currentJob_.outputPath)) {
        if (currentOptions_.existingFilePolicy != ExistingFilePolicy::Overwrite) {
            fail(QStringLiteral("The output file appeared while conversion was running."));
            return;
        }
        if (!QFile::remove(currentJob_.outputPath)) {
            fail(QStringLiteral("Could not replace the existing output file."));
            return;
        }
    }

    if (!QFile::rename(filePath, currentJob_.outputPath)) {
        fail(QStringLiteral("Could not move the completed PDF to its final location."));
        return;
    }
    finish(true);
}

void HtmlToPdfConverter::fail(const QString &message) {
    page_->triggerAction(QWebEnginePage::Stop);
    QFile::remove(temporaryPdfPath());
    finish(false, message);
}

void HtmlToPdfConverter::finish(bool success, const QString &message) {
    timeoutTimer_.stop();
    readinessTimer_.stop();
    settleTimer_.stop();

    const ConversionJob finishedJob = currentJob_;
    state_ = State::Idle;
    currentJob_ = {};
    emit completed(finishedJob, success, message);
}

QString HtmlToPdfConverter::temporaryPdfPath() const {
    const QFileInfo outputInfo(currentJob_.outputPath);
    return outputInfo.absoluteDir().filePath(
        QStringLiteral(".%1.quteconvert-part.pdf").arg(outputInfo.completeBaseName()));
}

} // namespace quteconvert
