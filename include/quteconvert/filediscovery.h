#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

namespace quteconvert {

class FileDiscovery {
public:
    static QStringList findHtmlFiles(const QString &directoryPath,
                                     QString *errorMessage = nullptr);
    static QString outputPathFor(const QString &inputPath,
                                 const QString &outputDirectory);
    static QString uniqueOutputPath(const QString &desiredPath,
                                    const QSet<QString> &reservedPaths = {});
};

} // namespace quteconvert
