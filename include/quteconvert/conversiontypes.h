#pragma once

#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QString>

namespace quteconvert {

enum class ExistingFilePolicy { Skip, Overwrite, AddSuffix };

struct ConversionOptions {
  QPageSize pageSize{QPageSize::A4};
  QPageLayout::Orientation orientation{QPageLayout::Portrait};
  QMarginsF marginsMm{10.0, 10.0, 10.0, 10.0};
  int loadTimeoutMs{120000};
  int settleDelayMs{1000};
  bool allowRemoteResources{false};
  ExistingFilePolicy existingFilePolicy{ExistingFilePolicy::Skip};
};

struct ConversionJob {
  QString inputPath;
  QString outputPath;
};

}  // namespace quteconvert
