#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTemporaryDir>

#include "quteconvert/filediscovery.h"

namespace {

void CreateFile(const QString& path) {
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("test");
}

TEST(FileDiscovery, FindsHtmlFilesNonRecursivelyInNaturalOrder) {
  QTemporaryDir temporary_directory;
  ASSERT_TRUE(temporary_directory.isValid());
  QDir directory(temporary_directory.path());

  CreateFile(directory.filePath(QStringLiteral("page10.html")));
  CreateFile(directory.filePath(QStringLiteral("page2.HTML")));
  CreateFile(directory.filePath(QStringLiteral("notes.txt")));
  ASSERT_TRUE(directory.mkpath(QStringLiteral("images")));
  CreateFile(directory.filePath(QStringLiteral("images/hidden.html")));

  QString error;
  const QStringList files = quteconvert::FileDiscovery::FindHtmlFiles(
      temporary_directory.path(), &error);

  EXPECT_TRUE(error.isEmpty());
  ASSERT_EQ(files.size(), 2);
  EXPECT_EQ(QFileInfo(files.at(0)).fileName(), QStringLiteral("page2.HTML"));
  EXPECT_EQ(QFileInfo(files.at(1)).fileName(), QStringLiteral("page10.html"));
}

TEST(FileDiscovery, ReportsMissingDirectory) {
  QString error;
  const QStringList files = quteconvert::FileDiscovery::FindHtmlFiles(
      QStringLiteral("Z:/this/directory/should/not/exist"), &error);

  EXPECT_TRUE(files.isEmpty());
  EXPECT_FALSE(error.isEmpty());
}

TEST(FileDiscovery, PreservesCompleteInputBaseName) {
  const QString output = quteconvert::FileDiscovery::OutputPathFor(
      QStringLiteral("C:/input/report.final.html"),
      QStringLiteral("C:/output"));

  EXPECT_EQ(QFileInfo(output).fileName(), QStringLiteral("report.final.pdf"));
}

TEST(FileDiscovery, AddsSuffixForExistingOrReservedOutputs) {
  QTemporaryDir temporary_directory;
  ASSERT_TRUE(temporary_directory.isValid());
  QDir directory(temporary_directory.path());
  const QString desired = directory.filePath(QStringLiteral("report.pdf"));
  CreateFile(desired);

  const QString second = quteconvert::FileDiscovery::UniqueOutputPath(desired);
  EXPECT_EQ(QFileInfo(second).fileName(), QStringLiteral("report (2).pdf"));

  const QSet<QString> reserved{QFileInfo(second).absoluteFilePath()};
  const QString third =
      quteconvert::FileDiscovery::UniqueOutputPath(desired, reserved);
  EXPECT_EQ(QFileInfo(third).fileName(), QStringLiteral("report (3).pdf"));
}

}  // namespace
