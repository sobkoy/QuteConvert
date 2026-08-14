#pragma once

#include <QMainWindow>
#include <QStringList>

#include "quteconvert/conversiontypes.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;

namespace quteconvert {

class ConversionController;

class MainWindow final : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);

 private:
  void ChooseInputDirectory();
  void ChooseOutputDirectory();
  void RefreshFiles();
  void StartConversion();
  void SetBatchRunning(bool running);
  ConversionOptions SelectedOptions() const;

  ConversionController* controller_{nullptr};
  QLineEdit* input_path_edit_{nullptr};
  QLineEdit* output_path_edit_{nullptr};
  QPushButton* input_browse_button_{nullptr};
  QPushButton* output_browse_button_{nullptr};
  QPushButton* refresh_button_{nullptr};
  QListWidget* file_list_{nullptr};
  QLabel* file_count_label_{nullptr};
  QComboBox* page_size_combo_{nullptr};
  QComboBox* orientation_combo_{nullptr};
  QDoubleSpinBox* margin_spin_{nullptr};
  QSpinBox* timeout_spin_{nullptr};
  QSpinBox* settle_delay_spin_{nullptr};
  QComboBox* existing_file_combo_{nullptr};
  QCheckBox* remote_resources_check_{nullptr};
  QPushButton* start_button_{nullptr};
  QPushButton* cancel_button_{nullptr};
  QProgressBar* progress_bar_{nullptr};
  QLabel* status_label_{nullptr};
  QPlainTextEdit* log_edit_{nullptr};
  QStringList input_files_;
};

}  // namespace quteconvert
