#include "quteconvert/filediscovery.h"

#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

namespace quteconvert {

QStringList FileDiscovery::findHtmlFiles(const QString& directoryPath,
                                         QString* errorMessage) {
  const QDir directory(directoryPath);
  if (!directory.exists()) {
    if (errorMessage) {
      *errorMessage = QStringLiteral("Input directory does not exist: %1")
                          .arg(directoryPath);
    }
    return {};
  }

  const QFileInfoList entries = directory.entryInfoList(
      {QStringLiteral("*.html"), QStringLiteral("*.htm")},
      QDir::Files | QDir::Readable | QDir::NoSymLinks, QDir::NoSort);

  QStringList files;
  files.reserve(entries.size());
  for (const QFileInfo& entry : entries) {
    files.append(entry.absoluteFilePath());
  }

  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setNumericMode(true);
  std::sort(files.begin(), files.end(),
            [&collator](const QString& left, const QString& right) {
              return collator.compare(QFileInfo(left).fileName(),
                                      QFileInfo(right).fileName()) < 0;
            });

  if (errorMessage) {
    errorMessage->clear();
  }
  return files;
}

QString FileDiscovery::outputPathFor(const QString& inputPath,
                                     const QString& outputDirectory) {
  const QFileInfo inputInfo(inputPath);
  return QDir(outputDirectory)
      .filePath(inputInfo.completeBaseName() + QStringLiteral(".pdf"));
}

QString FileDiscovery::uniqueOutputPath(const QString& desiredPath,
                                        const QSet<QString>& reservedPaths) {
  const QFileInfo desiredInfo(desiredPath);
  const QString normalizedDesired =
      QDir::cleanPath(desiredInfo.absoluteFilePath());
  if (!QFileInfo::exists(normalizedDesired) &&
      !reservedPaths.contains(normalizedDesired)) {
    return normalizedDesired;
  }

  const QDir directory = desiredInfo.absoluteDir();
  const QString baseName = desiredInfo.completeBaseName();
  const QString suffix = desiredInfo.suffix();
  for (int number = 2;; ++number) {
    const QString fileName =
        QStringLiteral("%1 (%2).%3").arg(baseName).arg(number).arg(suffix);
    const QString candidate = QDir::cleanPath(directory.filePath(fileName));
    if (!QFileInfo::exists(candidate) && !reservedPaths.contains(candidate)) {
      return candidate;
    }
  }
}

}  // namespace quteconvert
