# Third-Party Notices

QuteConvert is distributed with the Qt 6 runtime and Qt WebEngine when built with the open-source Qt distribution. The QuteConvert source code is licensed under the MIT License; Qt, Qt WebEngine, Chromium, and their bundled components are separate third-party works with their own licenses.

## Qt 6

QuteConvert uses Qt modules including Qt Core, GUI, Widgets, Network, PDF, Positioning, QML, Quick, SVG, WebChannel, and WebEngine. The open-source Qt distribution is available under the GNU Lesser General Public License version 3 (LGPL-3.0) and the GNU General Public License version 2 (GPL-2.0), subject to the licensing terms applicable to the installed Qt build.

- Qt licensing information: <https://www.qt.io/licensing/open-source-lgpl-obligations>
- Qt source code and license texts: <https://code.qt.io/cgit/qt/qtbase.git/tree/LICENSES>

## Qt WebEngine and Chromium

Qt WebEngine incorporates Chromium and additional third-party components. Their notices and license texts are included with the Qt WebEngine source and binary distribution.

- Qt WebEngine licensing information: <https://doc.qt.io/qt-6/qtwebengine-licensing.html>
- Chromium licenses: <https://chromium.googlesource.com/chromium/src/+/main/LICENSE>

## Release-maintainer checklist

Before publishing a Windows ZIP built with the open-source Qt distribution:

1. Copy the applicable Qt and Qt WebEngine license texts/notices from the exact Qt SDK version used to create the release into the ZIP, alongside this file.
2. Preserve the dynamic Qt library deployment produced by `windeployqt`; do not replace it with a statically linked Qt build unless you have separately reviewed the licensing obligations.
3. Provide access to the corresponding source code or source offer when required by the applicable third-party license.
4. Review the Qt licensing pages above and the notices shipped with the exact SDK before each release.

This document is a distribution checklist, not legal advice. If you need certainty about a commercial or closed-source distribution, obtain legal advice or use a Qt commercial license.
