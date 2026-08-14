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
  auto* main_layout = new QVBoxLayout(central);

  auto* input_layout = new QHBoxLayout;
  input_layout->addWidget(
      new QLabel(QStringLiteral("HTML directory:"), central));
  input_path_edit_ = new QLineEdit(central);
  input_path_edit_->setPlaceholderText(
      QStringLiteral("Directory containing HTML files"));
  input_browse_button_ = new QPushButton(QStringLiteral("Browse..."), central);
  refresh_button_ = new QPushButton(QStringLiteral("Refresh"), central);
  input_layout->addWidget(input_path_edit_, 1);
  input_layout->addWidget(input_browse_button_);
  input_layout->addWidget(refresh_button_);
  main_layout->addLayout(input_layout);

  auto* output_layout = new QHBoxLayout;
  output_layout->addWidget(
      new QLabel(QStringLiteral("PDF directory:"), central));
  output_path_edit_ = new QLineEdit(central);
  output_path_edit_->setPlaceholderText(
      QStringLiteral("Directory for generated PDFs"));
  output_browse_button_ = new QPushButton(QStringLiteral("Browse..."), central);
  output_layout->addWidget(output_path_edit_, 1);
  output_layout->addWidget(output_browse_button_);
  main_layout->addLayout(output_layout);

  file_count_label_ =
      new QLabel(QStringLiteral("No directory selected"), central);
  main_layout->addWidget(file_count_label_);
  file_list_ = new QListWidget(central);
  file_list_->setAlternatingRowColors(true);
  main_layout->addWidget(file_list_, 1);

  auto* advanced_group =
      new QGroupBox(QStringLiteral("PDF and loading options"), central);
  auto* advanced_layout = new QFormLayout(advanced_group);
  page_size_combo_ = new QComboBox(advanced_group);
  page_size_combo_->addItem(QStringLiteral("A4"), QPageSize::A4);
  page_size_combo_->addItem(QStringLiteral("Letter"), QPageSize::Letter);
  page_size_combo_->addItem(QStringLiteral("Legal"), QPageSize::Legal);
  advanced_layout->addRow(QStringLiteral("Page size:"), page_size_combo_);

  orientation_combo_ = new QComboBox(advanced_group);
  orientation_combo_->addItem(QStringLiteral("Portrait"),
                              QPageLayout::Portrait);
  orientation_combo_->addItem(QStringLiteral("Landscape"),
                              QPageLayout::Landscape);
  advanced_layout->addRow(QStringLiteral("Orientation:"), orientation_combo_);

  margin_spin_ = new QDoubleSpinBox(advanced_group);
  margin_spin_->setRange(0.0, 50.0);
  margin_spin_->setDecimals(1);
  margin_spin_->setSuffix(QStringLiteral(" mm"));
  margin_spin_->setValue(10.0);
  advanced_layout->addRow(QStringLiteral("All margins:"), margin_spin_);

  timeout_spin_ = new QSpinBox(advanced_group);
  timeout_spin_->setRange(10, 3600);
  timeout_spin_->setSuffix(QStringLiteral(" s"));
  timeout_spin_->setValue(120);
  advanced_layout->addRow(QStringLiteral("Load/print timeout:"), timeout_spin_);

  settle_delay_spin_ = new QSpinBox(advanced_group);
  settle_delay_spin_->setRange(0, 30000);
  settle_delay_spin_->setSingleStep(250);
  settle_delay_spin_->setSuffix(QStringLiteral(" ms"));
  settle_delay_spin_->setValue(1000);
  advanced_layout->addRow(QStringLiteral("Post-load settling delay:"),
                          settle_delay_spin_);

  existing_file_combo_ = new QComboBox(advanced_group);
  existing_file_combo_->addItem(QStringLiteral("Skip existing PDFs"),
                                static_cast<int>(ExistingFilePolicy::kSkip));
  existing_file_combo_->addItem(
      QStringLiteral("Overwrite existing PDFs"),
      static_cast<int>(ExistingFilePolicy::kOverwrite));
  existing_file_combo_->addItem(
      QStringLiteral("Add numeric suffix"),
      static_cast<int>(ExistingFilePolicy::kAddSuffix));
  advanced_layout->addRow(QStringLiteral("If output exists:"),
                          existing_file_combo_);

  remote_resources_check_ = new QCheckBox(
      QStringLiteral("Allow local HTML to load resources from the internet"),
      advanced_group);
  advanced_layout->addRow(remote_resources_check_);
  main_layout->addWidget(advanced_group);

  auto* action_layout = new QHBoxLayout;
  start_button_ = new QPushButton(QStringLiteral("Start conversion"), central);
  start_button_->setEnabled(false);
  cancel_button_ = new QPushButton(QStringLiteral("Cancel"), central);
  cancel_button_->setEnabled(false);
  action_layout->addWidget(start_button_);
  action_layout->addWidget(cancel_button_);
  action_layout->addStretch();
  main_layout->addLayout(action_layout);

  status_label_ = new QLabel(QStringLiteral("Ready"), central);
  progress_bar_ = new QProgressBar(central);
  progress_bar_->setRange(0, 1);
  progress_bar_->setValue(0);
  main_layout->addWidget(status_label_);
  main_layout->addWidget(progress_bar_);

  log_edit_ = new QPlainTextEdit(central);
  log_edit_->setReadOnly(true);
  log_edit_->setMaximumBlockCount(1000);
  log_edit_->setPlaceholderText(
      QStringLiteral("Conversion messages will appear here."));
  main_layout->addWidget(log_edit_, 1);
  setCentralWidget(central);

  connect(input_browse_button_, &QPushButton::clicked, this,
          &MainWindow::ChooseInputDirectory);
  connect(output_browse_button_, &QPushButton::clicked, this,
          &MainWindow::ChooseOutputDirectory);
  connect(refresh_button_, &QPushButton::clicked, this,
          &MainWindow::RefreshFiles);
  connect(input_path_edit_, &QLineEdit::editingFinished, this,
          &MainWindow::RefreshFiles);
  connect(start_button_, &QPushButton::clicked, this,
          &MainWindow::StartConversion);
  connect(cancel_button_, &QPushButton::clicked, controller_,
          &ConversionController::Cancel);

  connect(controller_, &ConversionController::BatchStarted, this,
          [this](int total) {
            progress_bar_->setRange(0, total);
            progress_bar_->setValue(0);
            SetBatchRunning(true);
          });
  connect(controller_, &ConversionController::FileStarted, this,
          [this](const QString& path, int current, int total) {
            status_label_->setText(QStringLiteral("%1 of %2: %3")
                                       .arg(current)
                                       .arg(total)
                                       .arg(QFileInfo(path).fileName()));
          });
  connect(controller_, &ConversionController::OverallProgress, this,
          [this](int completed, int total) {
            progress_bar_->setRange(0, total);
            progress_bar_->setValue(completed);
          });
  connect(controller_, &ConversionController::LogMessage, log_edit_,
          &QPlainTextEdit::appendPlainText);
  connect(controller_, &ConversionController::BatchFinished, this,
          [this](int succeeded, int failed, int skipped) {
            SetBatchRunning(false);
            status_label_->setText(
                QStringLiteral("Finished: %1 created, %2 failed, %3 skipped")
                    .arg(succeeded)
                    .arg(failed)
                    .arg(skipped));
          });
}

void MainWindow::ChooseInputDirectory() {
  const QString initial_path =
      input_path_edit_->text().isEmpty()
          ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
          : input_path_edit_->text();
  const QString path = QFileDialog::getExistingDirectory(
      this, QStringLiteral("Choose HTML directory"), initial_path);
  if (path.isEmpty()) {
    return;
  }
  input_path_edit_->setText(QDir::toNativeSeparators(path));
  if (output_path_edit_->text().isEmpty()) {
    output_path_edit_->setText(
        QDir::toNativeSeparators(QDir(path).filePath(QStringLiteral("PDF"))));
  }
  RefreshFiles();
}

void MainWindow::ChooseOutputDirectory() {
  const QString initial_path = output_path_edit_->text().isEmpty()
                                   ? input_path_edit_->text()
                                   : output_path_edit_->text();
  const QString path = QFileDialog::getExistingDirectory(
      this, QStringLiteral("Choose PDF directory"), initial_path);
  if (!path.isEmpty()) {
    output_path_edit_->setText(QDir::toNativeSeparators(path));
  }
}

void MainWindow::RefreshFiles() {
  QString error_message;
  input_files_ =
      FileDiscovery::FindHtmlFiles(input_path_edit_->text(), &error_message);
  file_list_->clear();
  for (const QString& path : input_files_) {
    file_list_->addItem(QFileInfo(path).fileName());
  }
  file_count_label_->setText(
      error_message.isEmpty()
          ? QStringLiteral("%1 HTML file(s), non-recursive scan")
                .arg(input_files_.size())
          : error_message);
  start_button_->setEnabled(!controller_->IsRunning() &&
                            !input_files_.isEmpty());
}

void MainWindow::StartConversion() {
  RefreshFiles();
  if (input_files_.isEmpty()) {
    QMessageBox::warning(
        this, QStringLiteral("No HTML files"),
        QStringLiteral("The selected directory contains no readable "
                       ".html or .htm files."));
    return;
  }
  if (output_path_edit_->text().trimmed().isEmpty()) {
    QMessageBox::warning(
        this, QStringLiteral("No output directory"),
        QStringLiteral("Choose a directory for generated PDFs."));
    return;
  }
  log_edit_->clear();
  controller_->Start(input_files_, output_path_edit_->text(),
                     SelectedOptions());
}

void MainWindow::SetBatchRunning(bool running) {
  input_path_edit_->setEnabled(!running);
  output_path_edit_->setEnabled(!running);
  input_browse_button_->setEnabled(!running);
  output_browse_button_->setEnabled(!running);
  refresh_button_->setEnabled(!running);
  start_button_->setEnabled(!running && !input_files_.isEmpty());
  cancel_button_->setEnabled(running);
}

ConversionOptions MainWindow::SelectedOptions() const {
  ConversionOptions options;
  options.page_size = QPageSize(static_cast<QPageSize::PageSizeId>(
      page_size_combo_->currentData().toInt()));
  options.orientation = static_cast<QPageLayout::Orientation>(
      orientation_combo_->currentData().toInt());
  const double margin = margin_spin_->value();
  options.margins_mm = QMarginsF(margin, margin, margin, margin);
  options.load_timeout_ms = timeout_spin_->value() * 1000;
  options.settle_delay_ms = settle_delay_spin_->value();
  options.allow_remote_resources = remote_resources_check_->isChecked();
  options.existing_file_policy = static_cast<ExistingFilePolicy>(
      existing_file_combo_->currentData().toInt());
  return options;
}

}  // namespace quteconvert
