#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QTextStream>
#include <QTimer>

#include "quteconvert/conversioncontroller.h"
#include "quteconvert/conversiontypes.h"
#include "quteconvert/filediscovery.h"
#include "quteconvert/mainwindow.h"

using namespace quteconvert;

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("QuteConvert"));
  QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
  QCoreApplication::setOrganizationName(QStringLiteral("QuteConvert"));

  QCommandLineParser parser;
  parser.setApplicationDescription(QStringLiteral(
      "Batch-convert local HTML documents to PDF using Qt WebEngine."));
  parser.addHelpOption();
  parser.addVersionOption();

  const QCommandLineOption input_option(
      {QStringLiteral("i"), QStringLiteral("input")},
      QStringLiteral("Directory containing HTML files."),
      QStringLiteral("directory"));
  const QCommandLineOption output_option(
      {QStringLiteral("o"), QStringLiteral("output")},
      QStringLiteral("Directory for generated PDF files."),
      QStringLiteral("directory"));
  const QCommandLineOption overwrite_option(
      QStringLiteral("overwrite"),
      QStringLiteral("Overwrite existing PDF files."));
  const QCommandLineOption add_suffix_option(
      QStringLiteral("add-suffix"),
      QStringLiteral("Add a numeric suffix when a PDF already exists."));
  const QCommandLineOption allow_remote_option(
      QStringLiteral("allow-remote"),
      QStringLiteral("Allow local HTML to load internet resources."));
  const QCommandLineOption timeout_option(
      QStringLiteral("timeout"),
      QStringLiteral("Load and print timeout in seconds (default: 120)."),
      QStringLiteral("seconds"), QStringLiteral("120"));
  const QCommandLineOption settle_option(
      QStringLiteral("settle-delay"),
      QStringLiteral(
          "Delay after resources are ready in milliseconds (default: 1000)."),
      QStringLiteral("milliseconds"), QStringLiteral("1000"));

  parser.addOptions({input_option, output_option, overwrite_option,
                     add_suffix_option, allow_remote_option, timeout_option,
                     settle_option});
  parser.process(app);

  const bool command_line_mode =
      parser.isSet(input_option) || parser.isSet(output_option);
  if (!command_line_mode) {
    MainWindow window;
    window.show();
    return app.exec();
  }

  QTextStream error_stream(stderr);
  if (!parser.isSet(input_option) || !parser.isSet(output_option)) {
    error_stream
        << "Both --input and --output are required in command-line mode.\n";
    return 2;
  }
  if (parser.isSet(overwrite_option) && parser.isSet(add_suffix_option)) {
    error_stream << "--overwrite and --add-suffix cannot be used together.\n";
    return 2;
  }

  bool timeout_valid = false;
  bool settle_valid = false;
  const int timeout_seconds =
      parser.value(timeout_option).toInt(&timeout_valid);
  const int settle_milliseconds =
      parser.value(settle_option).toInt(&settle_valid);
  if (!timeout_valid || timeout_seconds < 1 || !settle_valid ||
      settle_milliseconds < 0) {
    error_stream << "Timeout values must be valid non-negative numbers.\n";
    return 2;
  }

  QString discovery_error;
  const QStringList input_files = FileDiscovery::FindHtmlFiles(
      parser.value(input_option), &discovery_error);
  if (!discovery_error.isEmpty()) {
    error_stream << discovery_error << '\n';
    return 2;
  }
  if (input_files.isEmpty()) {
    error_stream
        << "No .html or .htm files were found in the input directory.\n";
    return 2;
  }

  ConversionOptions options;
  options.allow_remote_resources = parser.isSet(allow_remote_option);
  options.load_timeout_ms = timeout_seconds * 1000;
  options.settle_delay_ms = settle_milliseconds;
  if (parser.isSet(overwrite_option)) {
    options.existing_file_policy = ExistingFilePolicy::kOverwrite;
  } else if (parser.isSet(add_suffix_option)) {
    options.existing_file_policy = ExistingFilePolicy::kAddSuffix;
  }

  ConversionController controller;
  QTextStream output_stream(stdout);
  QObject::connect(&controller, &ConversionController::LogMessage, &app,
                   [&output_stream](const QString& message) {
                     output_stream << message << '\n';
                     output_stream.flush();
                   });
  QObject::connect(
      &controller, &ConversionController::BatchFinished, &app,
      [&app](int, int failed, int) { app.exit(failed == 0 ? 0 : 1); });

  const QString output_directory = parser.value(output_option);
  QTimer::singleShot(0, &app,
                     [&controller, input_files, output_directory, options] {
                       controller.Start(input_files, output_directory, options);
                     });
  return app.exec();
}
