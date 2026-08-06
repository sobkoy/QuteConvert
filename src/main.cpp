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

  const QCommandLineOption inputOption(
      {QStringLiteral("i"), QStringLiteral("input")},
      QStringLiteral("Directory containing HTML files."),
      QStringLiteral("directory"));
  const QCommandLineOption outputOption(
      {QStringLiteral("o"), QStringLiteral("output")},
      QStringLiteral("Directory for generated PDF files."),
      QStringLiteral("directory"));
  const QCommandLineOption overwriteOption(
      QStringLiteral("overwrite"),
      QStringLiteral("Overwrite existing PDF files."));
  const QCommandLineOption addSuffixOption(
      QStringLiteral("add-suffix"),
      QStringLiteral("Add a numeric suffix when a PDF already exists."));
  const QCommandLineOption allowRemoteOption(
      QStringLiteral("allow-remote"),
      QStringLiteral("Allow local HTML to load internet resources."));
  const QCommandLineOption timeoutOption(
      QStringLiteral("timeout"),
      QStringLiteral("Load and print timeout in seconds (default: 120)."),
      QStringLiteral("seconds"), QStringLiteral("120"));
  const QCommandLineOption settleOption(
      QStringLiteral("settle-delay"),
      QStringLiteral(
          "Delay after resources are ready in milliseconds (default: 1000)."),
      QStringLiteral("milliseconds"), QStringLiteral("1000"));

  parser.addOptions({inputOption, outputOption, overwriteOption,
                     addSuffixOption, allowRemoteOption, timeoutOption,
                     settleOption});
  parser.process(app);

  const bool commandLineMode =
      parser.isSet(inputOption) || parser.isSet(outputOption);
  if (!commandLineMode) {
    MainWindow window;
    window.show();
    return app.exec();
  }

  QTextStream errorStream(stderr);
  if (!parser.isSet(inputOption) || !parser.isSet(outputOption)) {
    errorStream
        << "Both --input and --output are required in command-line mode.\n";
    return 2;
  }
  if (parser.isSet(overwriteOption) && parser.isSet(addSuffixOption)) {
    errorStream << "--overwrite and --add-suffix cannot be used together.\n";
    return 2;
  }

  bool timeoutValid = false;
  bool settleValid = false;
  const int timeoutSeconds = parser.value(timeoutOption).toInt(&timeoutValid);
  const int settleMilliseconds = parser.value(settleOption).toInt(&settleValid);
  if (!timeoutValid || timeoutSeconds < 1 || !settleValid ||
      settleMilliseconds < 0) {
    errorStream << "Timeout values must be valid non-negative numbers.\n";
    return 2;
  }

  QString discoveryError;
  const QStringList inputFiles =
      FileDiscovery::findHtmlFiles(parser.value(inputOption), &discoveryError);
  if (!discoveryError.isEmpty()) {
    errorStream << discoveryError << '\n';
    return 2;
  }
  if (inputFiles.isEmpty()) {
    errorStream
        << "No .html or .htm files were found in the input directory.\n";
    return 2;
  }

  ConversionOptions options;
  options.allowRemoteResources = parser.isSet(allowRemoteOption);
  options.loadTimeoutMs = timeoutSeconds * 1000;
  options.settleDelayMs = settleMilliseconds;
  if (parser.isSet(overwriteOption)) {
    options.existingFilePolicy = ExistingFilePolicy::Overwrite;
  } else if (parser.isSet(addSuffixOption)) {
    options.existingFilePolicy = ExistingFilePolicy::AddSuffix;
  }

  ConversionController controller;
  QTextStream outputStream(stdout);
  QObject::connect(&controller, &ConversionController::logMessage, &app,
                   [&outputStream](const QString& message) {
                     outputStream << message << '\n';
                     outputStream.flush();
                   });
  QObject::connect(
      &controller, &ConversionController::batchFinished, &app,
      [&app](int, int failed, int) { app.exit(failed == 0 ? 0 : 1); });

  const QString outputDirectory = parser.value(outputOption);
  QTimer::singleShot(0, &app,
                     [&controller, inputFiles, outputDirectory, options] {
                       controller.start(inputFiles, outputDirectory, options);
                     });
  return app.exec();
}
