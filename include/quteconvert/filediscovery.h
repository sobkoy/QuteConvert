#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

namespace quteconvert {

class FileDiscovery {
 public:
  static QStringList FindHtmlFiles(const QString& directory_path,
                                   QString* error_message = nullptr);
  static QString OutputPathFor(const QString& input_path,
                               const QString& output_directory);
  static QString UniqueOutputPath(const QString& desired_path,
                                  const QSet<QString>& reserved_paths = {});
};

}  // namespace quteconvert
