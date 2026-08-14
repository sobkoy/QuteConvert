#include "quteconvert/filediscovery.h"

#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

namespace quteconvert {

QStringList FileDiscovery::FindHtmlFiles(const QString& directory_path,
                                         QString* error_message) {
  const QDir directory(directory_path);
  if (!directory.exists()) {
    if (error_message) {
      *error_message = QStringLiteral("Input directory does not exist: %1")
                           .arg(directory_path);
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

  if (error_message) {
    error_message->clear();
  }
  return files;
}

QString FileDiscovery::OutputPathFor(const QString& input_path,
                                     const QString& output_directory) {
  const QFileInfo input_info(input_path);
  return QDir(output_directory)
      .filePath(input_info.completeBaseName() + QStringLiteral(".pdf"));
}

QString FileDiscovery::UniqueOutputPath(const QString& desired_path,
                                        const QSet<QString>& reserved_paths) {
  const QFileInfo desired_info(desired_path);
  const QString normalized_desired =
      QDir::cleanPath(desired_info.absoluteFilePath());
  if (!QFileInfo::exists(normalized_desired) &&
      !reserved_paths.contains(normalized_desired)) {
    return normalized_desired;
  }

  const QDir directory = desired_info.absoluteDir();
  const QString base_name = desired_info.completeBaseName();
  const QString suffix = desired_info.suffix();
  for (int number = 2;; ++number) {
    const QString file_name =
        QStringLiteral("%1 (%2).%3").arg(base_name).arg(number).arg(suffix);
    const QString candidate = QDir::cleanPath(directory.filePath(file_name));
    if (!QFileInfo::exists(candidate) && !reserved_paths.contains(candidate)) {
      return candidate;
    }
  }
}

}  // namespace quteconvert
