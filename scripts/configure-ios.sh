#!/bin/sh
# Use the host binary directly: generated Qt wrappers do not quote paths with spaces.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk=${WFVIEW_QT_ROOT:-/Volumes/1TbSilver/wfview-ios}
version=${WFVIEW_QT_VERSION:-6.8.3}
platform=${WFVIEW_IOS_PLATFORM:-iphoneos}
case "$platform" in
    iphoneos) arch=arm64; build_name=wfview-device ;;
    iphonesimulator) arch=${WFVIEW_SIM_ARCH:-x86_64}; build_name=wfview-simulator ;;
    *) echo 'WFVIEW_IOS_PLATFORM must be iphoneos or iphonesimulator' >&2; exit 2 ;;
esac
build_dir="$sdk/build/$build_name"
qmake="$sdk/Qt/$version/macos/bin/qmake6"
qtconf="$sdk/Qt/$version/ios/bin/target_qt.conf"
if ! test -d "$sdk"; then
    printf 'iOS dependency directory is unavailable: %s\nConnect the dependency disk or set WFVIEW_QT_ROOT.\n' "$sdk" >&2
    exit 1
fi
if ! test -x "$qmake" || ! test -f "$qtconf"; then
    printf 'Qt %s for macOS/iOS is incomplete in %s\nRun scripts/install-ios-qt.sh with the same WFVIEW_QT_ROOT.\n' "$version" "$sdk" >&2
    exit 1
fi
for required in "install/$platform-$arch/lib/libopus.a" \
                src/eigen-3.4.0/Eigen/Core \
                src/r8brain-free-src-version-6.5/CDSPResampler.h \
                src/qcustomplot-source/qcustomplot.cpp; do
    if ! test -f "$sdk/$required"; then
        printf 'Missing iOS dependency: %s\n' "$sdk/$required" >&2
        exit 1
    fi
done
mkdir -p "$build_dir"
cd "$build_dir"
exec "$qmake" -qtconf "$qtconf" "$root/wfview.pro" -spec macx-ios-clang CONFIG+=release "CONFIG+=$platform" "WFVIEW_IOS_ROOT=$sdk" "WFVIEW_IOS_PLATFORM=$platform" "WFVIEW_IOS_ARCH=$arch" "QMAKE_APPLE_DEVICE_ARCHS=$arch" "QMAKE_APPLE_SIMULATOR_ARCHS=$arch" "$@"
