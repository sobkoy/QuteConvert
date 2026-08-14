#pragma once

#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QString>

namespace quteconvert {

enum class ExistingFilePolicy { kSkip, kOverwrite, kAddSuffix };

struct ConversionOptions {
  QPageSize page_size{QPageSize::A4};
  QPageLayout::Orientation orientation{QPageLayout::Portrait};
  QMarginsF margins_mm{10.0, 10.0, 10.0, 10.0};
  int load_timeout_ms{120000};
  int settle_delay_ms{1000};
  bool allow_remote_resources{false};
  ExistingFilePolicy existing_file_policy{ExistingFilePolicy::kSkip};
};

struct ConversionJob {
  QString input_path;
  QString output_path;
};

}  // namespace quteconvert
