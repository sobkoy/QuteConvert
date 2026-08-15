#include <gtest/gtest.h>

#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QMetaObject>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QWebEnginePage>

#include "quteconvert/conversioncontroller.h"
#include "quteconvert/htmltopdfconverter.h"

namespace {

constexpr int kTestTimeoutMs = 60000;

struct BatchResult {
  int succeeded{0};
  int failed{0};
  int skipped{0};
  bool finished{false};
  bool converter_released{false};
};

QString FixturePath() {
  return QDir(QStringLiteral(QUTECONVERT_INTEGRATION_FIXTURE_DIR))
      .filePath(QStringLiteral("sample.html"));
}

BatchResult RunBatch(quteconvert::ConversionController* controller,
                     const QStringList& input_files,
                     const QString& output_directory,
                     const quteconvert::ConversionOptions& options,
                     bool cancel_active_file = false) {
  QEventLoop event_loop;
  QTimer timeout_timer;
  timeout_timer.setSingleShot(true);

  BatchResult result;
  const QMetaObject::Connection finished_connection = QObject::connect(
      controller, &quteconvert::ConversionController::BatchFinished,
      &event_loop, [&](int succeeded, int failed, int skipped) {
        result.succeeded = succeeded;
        result.failed = failed;
        result.skipped = skipped;
        result.finished = true;
        result.converter_released =
            controller->findChildren<quteconvert::HtmlToPdfConverter*>()
                .isEmpty();
        event_loop.quit();
      });
  QObject::connect(&timeout_timer, &QTimer::timeout, &event_loop,
                   &QEventLoop::quit);

  QMetaObject::Connection cancel_connection;
  if (cancel_active_file) {
    cancel_connection = QObject::connect(
        controller, &quteconvert::ConversionController::FileStarted, controller,
        [controller] {
          QTimer::singleShot(0, controller,
                             &quteconvert::ConversionController::Cancel);
        },
        Qt::SingleShotConnection);
  }

  timeout_timer.start(kTestTimeoutMs);
  controller->Start(input_files, output_directory, options);
  event_loop.exec();

  QObject::disconnect(finished_connection);
  QObject::disconnect(cancel_connection);
  return result;
}

TEST(HtmlToPdfConverterLifecycle, ReleasesPageBeforeCompletedSignal) {
  QTemporaryDir output_directory;
  ASSERT_TRUE(output_directory.isValid());

  quteconvert::HtmlToPdfConverter converter;
  quteconvert::ConversionOptions options;
  options.settle_delay_ms = 0;
  const quteconvert::ConversionJob job{
      FixturePath(),
      QDir(output_directory.path()).filePath(QStringLiteral("sample.pdf"))};

  QEventLoop event_loop;
  QTimer timeout_timer;
  timeout_timer.setSingleShot(true);
  bool completed = false;
  bool page_released = false;
  bool conversion_succeeded = false;

  QObject::connect(
      &converter, &quteconvert::HtmlToPdfConverter::Completed, &event_loop,
      [&](const quteconvert::ConversionJob&, bool success, const QString&) {
        completed = true;
        conversion_succeeded = success;
        page_released = converter.findChildren<QWebEnginePage*>().isEmpty();
        event_loop.quit();
      });
  QObject::connect(&timeout_timer, &QTimer::timeout, &event_loop,
                   &QEventLoop::quit);

  timeout_timer.start(kTestTimeoutMs);
  converter.Convert(job, options);
  event_loop.exec();

  ASSERT_TRUE(completed);
  EXPECT_TRUE(conversion_succeeded);
  EXPECT_TRUE(page_released);
  EXPECT_TRUE(QFileInfo::exists(job.output_path));
  EXPECT_GT(QFileInfo(job.output_path).size(), 0);
}

TEST(ConversionControllerLifecycle, RecreatesProfileAcrossTenBatches) {
  QTemporaryDir output_root;
  ASSERT_TRUE(output_root.isValid());
  QDir output_directory(output_root.path());

  quteconvert::ConversionController controller;
  quteconvert::ConversionOptions options;
  options.settle_delay_ms = 0;
  options.existing_file_policy = quteconvert::ExistingFilePolicy::kOverwrite;

  constexpr int kBatchCount = 10;
  for (int batch_index = 0; batch_index < kBatchCount; ++batch_index) {
    const QString batch_name = QStringLiteral("batch-%1").arg(batch_index);
    ASSERT_TRUE(output_directory.mkpath(batch_name));
    const QString batch_output = output_directory.filePath(batch_name);
    const QStringList input_files =
        batch_index == 0 ? QStringList{FixturePath(), FixturePath()}
                         : QStringList{FixturePath()};

    const BatchResult result =
        RunBatch(&controller, input_files, batch_output, options);
    ASSERT_TRUE(result.finished);
    EXPECT_EQ(result.succeeded, input_files.size());
    EXPECT_EQ(result.failed, 0);
    EXPECT_EQ(result.skipped, 0);
    EXPECT_TRUE(result.converter_released);
    EXPECT_GT(
        QFileInfo(QDir(batch_output).filePath(QStringLiteral("sample.pdf")))
            .size(),
        0);
  }

  EXPECT_GT(QFileInfo(output_directory.filePath(
                          QStringLiteral("batch-0/sample (2).pdf")))
                .size(),
            0);
}

TEST(ConversionControllerLifecycle, ReleasesProfileAfterActiveCancellation) {
  QTemporaryDir output_directory;
  ASSERT_TRUE(output_directory.isValid());

  quteconvert::ConversionController controller;
  quteconvert::ConversionOptions options;
  options.settle_delay_ms = 5000;

  const BatchResult result = RunBatch(&controller, {FixturePath()},
                                      output_directory.path(), options, true);

  ASSERT_TRUE(result.finished);
  EXPECT_EQ(result.succeeded, 0);
  EXPECT_EQ(result.failed, 0);
  EXPECT_EQ(result.skipped, 0);
  EXPECT_TRUE(result.converter_released);
}

TEST(ConversionControllerLifecycle, ReleasesProfileAfterLoadFailure) {
  QTemporaryDir output_directory;
  ASSERT_TRUE(output_directory.isValid());

  quteconvert::ConversionController controller;
  quteconvert::ConversionOptions options;
  options.settle_delay_ms = 0;
  const QString missing_input =
      QDir(output_directory.path()).filePath(QStringLiteral("missing.html"));

  const BatchResult result =
      RunBatch(&controller, {missing_input}, output_directory.path(), options);

  ASSERT_TRUE(result.finished);
  EXPECT_EQ(result.succeeded, 0);
  EXPECT_EQ(result.failed, 1);
  EXPECT_EQ(result.skipped, 0);
  EXPECT_TRUE(result.converter_released);
}

}  // namespace
