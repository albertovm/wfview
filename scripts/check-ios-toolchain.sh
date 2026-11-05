#!/bin/sh
# Pass the qmake executable from the Qt for iOS installation.
set -eu
qmake_bin=${1:-qmake}
command -v "$qmake_bin" >/dev/null 2>&1 || { echo 'Qt qmake not found' >&2; exit 1; }
spec=$("$qmake_bin" -query QMAKE_XSPEC)
case "$spec" in
    *ios*) ;;
    *) echo "Unsupported target: $spec. Supply Qt for iOS qmake." >&2; exit 1 ;;
esac
xcrun --sdk iphoneos --show-sdk-path
printf '%s\n' 'iOS toolchain found. This does not validate application portability.'
