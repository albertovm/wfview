#!/bin/sh
# Isolated Qt SDK installation; does not modify the system Qt.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk=${WFVIEW_QT_ROOT:-"$root/.ios-sdk"}
version=${WFVIEW_QT_VERSION:-6.8.3}
mirror=${WFVIEW_QT_MIRROR:-https://www.nic.funet.fi/pub/mirrors/download.qt-project.org/}
# SMB cannot host the Unix sockets used by multiprocessing.
# Keep small control files local and large archives on the SDK volume.
mkdir -p "$sdk/tmp"
export TMPDIR=/private/tmp
python3 -m venv "$sdk/tools"
"$sdk/tools/bin/python" -m pip install 'aqtinstall==3.3.0'
"$sdk/tools/bin/python" -m aqt install-qt mac desktop "$version" clang_64 -O "$sdk/Qt" --archive-dest "$sdk/tmp" --timeout 60 -b "$mirror" -m qtmultimedia qtwebsockets
"$sdk/tools/bin/python" -m aqt install-qt mac ios "$version" ios -O "$sdk/Qt" --archive-dest "$sdk/tmp" --timeout 60 -b "$mirror" -m qtmultimedia qtwebsockets
