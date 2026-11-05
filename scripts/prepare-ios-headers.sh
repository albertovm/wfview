#!/bin/sh
set -eu
sdk=${WFVIEW_QT_ROOT:-/Volumes/1TbSilver/wfview-ios}
mkdir -p "$sdk/src"
for archive in QCustomPlot-source.tar.gz eigen-3.4.0.tar.gz r8brain-6.5.tar.gz; do
    test -f "$sdk/deps/$archive" || { echo "Missing dependency: $archive" >&2; exit 1; }
    tar -xzf "$sdk/deps/$archive" -C "$sdk/src"
done
