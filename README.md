# QuteConvert

QuteConvert is a Windows desktop and command-line application that batch-converts local HTML files to PDF using C++20, Qt 6, and Qt WebEngine.

## Features

- Converts `.html` and `.htm` files from a selected directory into PDFs.
- Provides both a graphical interface and command-line interface.
- Supports A4, Letter, and Legal paper sizes; portrait or landscape orientation; and configurable margins.
- Handles existing output files by skipping, overwriting, or adding a numeric suffix.
- Reports conversion progress and logs, and lets you cancel a running batch.

## Download and run (Windows x64)

1. Download the `QuteConvert-<version>-windows-x64.zip` file from the project's Releases page.
2. Extract the **entire** ZIP archive to a folder on your computer.
3. Run `QuteConvert.exe`.

Do not move `QuteConvert.exe` outside the extracted folder. The application needs the Qt DLLs, `QtWebEngineProcess.exe`, plugins, and `resources` folders distributed alongside it. You do not need to install Qt or compile the application to use a release.

## Using the graphical interface

1. Choose an **HTML directory** containing the files to convert. QuteConvert scans that directory only; it does not scan subdirectories.
2. Choose a **PDF directory** for the generated files.
3. Review the detected HTML files and configure any PDF or loading options:
   - page size: A4, Letter, or Legal;
   - orientation: portrait or landscape;
   - one margin value for all sides;
   - load/print timeout and post-load settling delay;
   - behavior when a PDF already exists: skip, overwrite, or add a numeric suffix;
   - optional permission for local HTML to load resources from the internet.
4. Select **Start conversion**. Follow the progress bar and log; select **Cancel** to stop the batch.

## Command-line usage

Run `QuteConvert.exe` from a Command Prompt or PowerShell window in the extracted application folder.

```powershell
.\QuteConvert.exe --input "C:\Documents\html" --output "C:\Documents\pdf"
```

By default, existing PDFs are skipped. Examples:

```powershell
# Replace existing PDFs.
.\QuteConvert.exe --input "C:\Documents\html" --output "C:\Documents\pdf" --overwrite

# Keep existing PDFs and create names such as "report (2).pdf".
.\QuteConvert.exe --input "C:\Documents\html" --output "C:\Documents\pdf" --add-suffix

# Allow external resources and adjust loading behavior.
.\QuteConvert.exe --input "C:\Documents\html" --output "C:\Documents\pdf" --allow-remote --timeout 180 --settle-delay 1500
```

Available options:

| Option | Description |
| --- | --- |
| `-i`, `--input <directory>` | Directory containing HTML files. |
| `-o`, `--output <directory>` | Directory for generated PDFs. |
| `--overwrite` | Replace existing PDFs. Cannot be combined with `--add-suffix`. |
| `--add-suffix` | Add a numeric suffix to duplicate names. |
| `--allow-remote` | Allow local HTML documents to fetch internet resources. |
| `--timeout <seconds>` | Load/print timeout; default: `120`. |
| `--settle-delay <milliseconds>` | Extra delay after document readiness; default: `1000`. |
| `--help` | Show command-line help. |
| `--version` | Show the application version. |

## Privacy and security

QuteConvert processes the selected HTML files locally on your computer. By default, local HTML documents cannot load resources from the internet. Enable **Allow local HTML to load resources from the internet** (or `--allow-remote`) only for files you trust and only when external CSS, images, fonts, or scripts are required.

## Build from source (Windows x64)

### Requirements

- Windows x64
- Visual Studio Build Tools or Visual Studio with the MSVC C++ compiler and C++20 support
- CMake 3.22 or newer
- Qt 6.11.1 for MSVC 2022 x64, including the Qt WebEngine module

The checked-in preset expects Qt at `C:\Qt\6.11.1\msvc2022_64`. To use another Qt installation, provide an appropriate `CMAKE_PREFIX_PATH` when configuring CMake or update a local `CMakeUserPresets.json` file.

### Configure and build

```powershell
cmake --preset qt-6.11.1
cmake --build --preset qt-6.11.1-release
```

The Release executable is written to `build\Release\QuteConvert.exe`. On Windows, the project runs `windeployqt` after the build to deploy the required Qt runtime files beside the executable.

### Run tests

```powershell
cmake --build --preset qt-6.11.1-debug
ctest --test-dir build -C Debug --output-on-failure
```

## License

QuteConvert is available under the [MIT License](LICENSE).

This project uses Qt and Qt WebEngine. Release packages must include the applicable third-party notices and license texts; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

