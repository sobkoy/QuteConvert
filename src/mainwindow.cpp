#include "quteconvert/mainwindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPageSize>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>

#include "quteconvert/conversioncontroller.h"
#include "quteconvert/filediscovery.h"

namespace quteconvert {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), controller_(new ConversionController(this)) {
  setWindowTitle(QStringLiteral("QuteConvert"));
  resize(820, 720);

  auto* central = new QWidget(this);
  auto* mainLayout = new QVBoxLayout(central);

  auto* inputLayout = new QHBoxLayout;
  inputLayout->addWidget(
      new QLabel(QStringLiteral("HTML directory:"), central));
  inputPathEdit_ = new QLineEdit(central);
  inputPathEdit_->setPlaceholderText(
      QStringLiteral("Directory containing HTML files"));
  inputBrowseButton_ = new QPushButton(QStringLiteral("Browse..."), central);
  refreshButton_ = new QPushButton(QStringLiteral("Refresh"), central);
  inputLayout->addWidget(inputPathEdit_, 1);
  inputLayout->addWidget(inputBrowseButton_);
  inputLayout->addWidget(refreshButton_);
  mainLayout->addLayout(inputLayout);

  auto* outputLayout = new QHBoxLayout;
  outputLayout->addWidget(
      new QLabel(QStringLiteral("PDF directory:"), central));
  outputPathEdit_ = new QLineEdit(central);
  outputPathEdit_->setPlaceholderText(
      QStringLiteral("Directory for generated PDFs"));
  outputBrowseButton_ = new QPushButton(QStringLiteral("Browse..."), central);
  outputLayout->addWidget(outputPathEdit_, 1);
  outputLayout->addWidget(outputBrowseButton_);
  mainLayout->addLayout(outputLayout);

  fileCountLabel_ =
      new QLabel(QStringLiteral("No directory selected"), central);
  mainLayout->addWidget(fileCountLabel_);
  fileList_ = new QListWidget(central);
  fileList_->setAlternatingRowColors(true);
  mainLayout->addWidget(fileList_, 1);

  auto* advancedGroup =
      new QGroupBox(QStringLiteral("PDF and loading options"), central);
  auto* advancedLayout = new QFormLayout(advancedGroup);
  pageSizeCombo_ = new QComboBox(advancedGroup);
  pageSizeCombo_->addItem(QStringLiteral("A4"), QPageSize::A4);
  pageSizeCombo_->addItem(QStringLiteral("Letter"), QPageSize::Letter);
  pageSizeCombo_->addItem(QStringLiteral("Legal"), QPageSize::Legal);
  advancedLayout->addRow(QStringLiteral("Page size:"), pageSizeCombo_);

  orientationCombo_ = new QComboBox(advancedGroup);
  orientationCombo_->addItem(QStringLiteral("Portrait"), QPageLayout::Portrait);
  orientationCombo_->addItem(QStringLiteral("Landscape"),
                             QPageLayout::Landscape);
  advancedLayout->addRow(QStringLiteral("Orientation:"), orientationCombo_);

  marginSpin_ = new QDoubleSpinBox(advancedGroup);
  marginSpin_->setRange(0.0, 50.0);
  marginSpin_->setDecimals(1);
  marginSpin_->setSuffix(QStringLiteral(" mm"));
  marginSpin_->setValue(10.0);
  advancedLayout->addRow(QStringLiteral("All margins:"), marginSpin_);

  timeoutSpin_ = new QSpinBox(advancedGroup);
  timeoutSpin_->setRange(10, 3600);
  timeoutSpin_->setSuffix(QStringLiteral(" s"));
  timeoutSpin_->setValue(120);
  advancedLayout->addRow(QStringLiteral("Load/print timeout:"), timeoutSpin_);

  settleDelaySpin_ = new QSpinBox(advancedGroup);
  settleDelaySpin_->setRange(0, 30000);
  settleDelaySpin_->setSingleStep(250);
  settleDelaySpin_->setSuffix(QStringLiteral(" ms"));
  settleDelaySpin_->setValue(1000);
  advancedLayout->addRow(QStringLiteral("Post-load settling delay:"),
                         settleDelaySpin_);

  existingFileCombo_ = new QComboBox(advancedGroup);
  existingFileCombo_->addItem(QStringLiteral("Skip existing PDFs"),
                              static_cast<int>(ExistingFilePolicy::Skip));
  existingFileCombo_->addItem(QStringLiteral("Overwrite existing PDFs"),
                              static_cast<int>(ExistingFilePolicy::Overwrite));
  existingFileCombo_->addItem(QStringLiteral("Add numeric suffix"),
                              static_cast<int>(ExistingFilePolicy::AddSuffix));
  advancedLayout->addRow(QStringLiteral("If output exists:"),
                         existingFileCombo_);

  remoteResourcesCheck_ = new QCheckBox(
      QStringLiteral("Allow local HTML to load resources from the internet"),
      advancedGroup);
  advancedLayout->addRow(remoteResourcesCheck_);
  mainLayout->addWidget(advancedGroup);

  auto* actionLayout = new QHBoxLayout;
  startButton_ = new QPushButton(QStringLiteral("Start conversion"), central);
  startButton_->setEnabled(false);
  cancelButton_ = new QPushButton(QStringLiteral("Cancel"), central);
  cancelButton_->setEnabled(false);
  actionLayout->addWidget(startButton_);
  actionLayout->addWidget(cancelButton_);
  actionLayout->addStretch();
  mainLayout->addLayout(actionLayout);

  statusLabel_ = new QLabel(QStringLiteral("Ready"), central);
  progressBar_ = new QProgressBar(central);
  progressBar_->setRange(0, 1);
  progressBar_->setValue(0);
  mainLayout->addWidget(statusLabel_);
  mainLayout->addWidget(progressBar_);

  logEdit_ = new QPlainTextEdit(central);
  logEdit_->setReadOnly(true);
  logEdit_->setMaximumBlockCount(1000);
  logEdit_->setPlaceholderText(
      QStringLiteral("Conversion messages will appear here."));
  mainLayout->addWidget(logEdit_, 1);
  setCentralWidget(central);

  connect(inputBrowseButton_, &QPushButton::clicked, this,
          &MainWindow::chooseInputDirectory);
  connect(outputBrowseButton_, &QPushButton::clicked, this,
          &MainWindow::chooseOutputDirectory);
  connect(refreshButton_, &QPushButton::clicked, this,
          &MainWindow::refreshFiles);
  connect(inputPathEdit_, &QLineEdit::editingFinished, this,
          &MainWindow::refreshFiles);
  connect(startButton_, &QPushButton::clicked, this,
          &MainWindow::startConversion);
  connect(cancelButton_, &QPushButton::clicked, controller_,
          &ConversionController::cancel);

  connect(controller_, &ConversionController::batchStarted, this,
          [this](int total) {
            progressBar_->setRange(0, total);
            progressBar_->setValue(0);
            setBatchRunning(true);
          });
  connect(controller_, &ConversionController::fileStarted, this,
          [this](const QString& path, int current, int total) {
            statusLabel_->setText(QStringLiteral("%1 of %2: %3")
                                      .arg(current)
                                      .arg(total)
                                      .arg(QFileInfo(path).fileName()));
          });
  connect(controller_, &ConversionController::overallProgress, this,
          [this](int completed, int total) {
            progressBar_->setRange(0, total);
            progressBar_->setValue(completed);
          });
  connect(controller_, &ConversionController::logMessage, logEdit_,
          &QPlainTextEdit::appendPlainText);
  connect(controller_, &ConversionController::batchFinished, this,
          [this](int succeeded, int failed, int skipped) {
            setBatchRunning(false);
            statusLabel_->setText(
                QStringLiteral("Finished: %1 created, %2 failed, %3 skipped")
                    .arg(succeeded)
                    .arg(failed)
                    .arg(skipped));
          });
}

void MainWindow::chooseInputDirectory() {
  const QString initialPath =
      inputPathEdit_->text().isEmpty()
          ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
          : inputPathEdit_->text();
  const QString path = QFileDialog::getExistingDirectory(
      this, QStringLiteral("Choose HTML directory"), initialPath);
  if (path.isEmpty()) {
    return;
  }
  inputPathEdit_->setText(QDir::toNativeSeparators(path));
  if (outputPathEdit_->text().isEmpty()) {
    outputPathEdit_->setText(
        QDir::toNativeSeparators(QDir(path).filePath(QStringLiteral("PDF"))));
  }
  refreshFiles();
}

void MainWindow::chooseOutputDirectory() {
  const QString initialPath = outputPathEdit_->text().isEmpty()
                                  ? inputPathEdit_->text()
                                  : outputPathEdit_->text();
  const QString path = QFileDialog::getExistingDirectory(
      this, QStringLiteral("Choose PDF directory"), initialPath);
  if (!path.isEmpty()) {
    outputPathEdit_->setText(QDir::toNativeSeparators(path));
  }
}

void MainWindow::refreshFiles() {
  QString errorMessage;
  inputFiles_ =
      FileDiscovery::findHtmlFiles(inputPathEdit_->text(), &errorMessage);
  fileList_->clear();
  for (const QString& path : inputFiles_) {
    fileList_->addItem(QFileInfo(path).fileName());
  }
  fileCountLabel_->setText(
      errorMessage.isEmpty()
          ? QStringLiteral("%1 HTML file(s), non-recursive scan")
                .arg(inputFiles_.size())
          : errorMessage);
  startButton_->setEnabled(!controller_->isRunning() && !inputFiles_.isEmpty());
}

void MainWindow::startConversion() {
  refreshFiles();
  if (inputFiles_.isEmpty()) {
    QMessageBox::warning(
        this, QStringLiteral("No HTML files"),
        QStringLiteral("The selected directory contains no readable "
                       ".html or .htm files."));
    return;
  }
  if (outputPathEdit_->text().trimmed().isEmpty()) {
    QMessageBox::warning(
        this, QStringLiteral("No output directory"),
        QStringLiteral("Choose a directory for generated PDFs."));
    return;
  }
  logEdit_->clear();
  controller_->start(inputFiles_, outputPathEdit_->text(), selectedOptions());
}

void MainWindow::setBatchRunning(bool running) {
  inputPathEdit_->setEnabled(!running);
  outputPathEdit_->setEnabled(!running);
  inputBrowseButton_->setEnabled(!running);
  outputBrowseButton_->setEnabled(!running);
  refreshButton_->setEnabled(!running);
  startButton_->setEnabled(!running && !inputFiles_.isEmpty());
  cancelButton_->setEnabled(running);
}

ConversionOptions MainWindow::selectedOptions() const {
  ConversionOptions options;
  options.pageSize = QPageSize(static_cast<QPageSize::PageSizeId>(
      pageSizeCombo_->currentData().toInt()));
  options.orientation = static_cast<QPageLayout::Orientation>(
      orientationCombo_->currentData().toInt());
  const double margin = marginSpin_->value();
  options.marginsMm = QMarginsF(margin, margin, margin, margin);
  options.loadTimeoutMs = timeoutSpin_->value() * 1000;
  options.settleDelayMs = settleDelaySpin_->value();
  options.allowRemoteResources = remoteResourcesCheck_->isChecked();
  options.existingFilePolicy = static_cast<ExistingFilePolicy>(
      existingFileCombo_->currentData().toInt());
  return options;
}

}  // namespace quteconvert
