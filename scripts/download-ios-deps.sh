#!/bin/sh
set -eu
sdk=${WFVIEW_QT_ROOT:-/Volumes/1TbSilver/wfview-ios}
mkdir -p "$sdk/deps"
fetch() {
    name=$1
    url=$2
    target="$sdk/deps/$name"
    if test -s "$target" && tar -tzf "$target" >/dev/null 2>&1; then
        printf 'Verified: %s\n' "$target"
        return
    fi
    curl -fL --connect-timeout 30 --max-time 1800 --retry 5 --retry-all-errors "$url" -o "$target.part"
    tar -tzf "$target.part" >/dev/null
    mv "$target.part" "$target"
}
fetch opus-1.5.2.tar.gz https://downloads.xiph.org/releases/opus/opus-1.5.2.tar.gz
fetch QCustomPlot-source.tar.gz https://www.qcustomplot.com/release/2.1.1/QCustomPlot-source.tar.gz
fetch eigen-3.4.0.tar.gz https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
fetch r8brain-6.5.tar.gz https://codeload.github.com/avaneev/r8brain-free-src/tar.gz/refs/tags/version-6.5
