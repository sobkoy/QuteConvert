#pragma once

#include "quteconvert/conversiontypes.h"

#include <QMainWindow>
#include <QStringList>

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
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void chooseInputDirectory();
    void chooseOutputDirectory();
    void refreshFiles();
    void startConversion();
    void setBatchRunning(bool running);
    ConversionOptions selectedOptions() const;

    ConversionController *controller_{nullptr};
    QLineEdit *inputPathEdit_{nullptr};
    QLineEdit *outputPathEdit_{nullptr};
    QPushButton *inputBrowseButton_{nullptr};
    QPushButton *outputBrowseButton_{nullptr};
    QPushButton *refreshButton_{nullptr};
    QListWidget *fileList_{nullptr};
    QLabel *fileCountLabel_{nullptr};
    QComboBox *pageSizeCombo_{nullptr};
    QComboBox *orientationCombo_{nullptr};
    QDoubleSpinBox *marginSpin_{nullptr};
    QSpinBox *timeoutSpin_{nullptr};
    QSpinBox *settleDelaySpin_{nullptr};
    QComboBox *existingFileCombo_{nullptr};
    QCheckBox *remoteResourcesCheck_{nullptr};
    QPushButton *startButton_{nullptr};
    QPushButton *cancelButton_{nullptr};
    QProgressBar *progressBar_{nullptr};
    QLabel *statusLabel_{nullptr};
    QPlainTextEdit *logEdit_{nullptr};
    QStringList inputFiles_;
};

} // namespace quteconvert
