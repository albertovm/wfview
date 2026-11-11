#!/bin/sh
# Build a static Opus library for an iOS device or simulator.
set -eu
sdk=${WFVIEW_QT_ROOT:-/Volumes/1TbSilver/wfview-ios}
platform=${1:-iphoneos}
case "$platform" in
    iphoneos) arch=arm64 ;;
    iphonesimulator) arch=${WFVIEW_SIM_ARCH:-x86_64} ;;
    *) echo 'Usage: build-ios-opus.sh [iphoneos|iphonesimulator]' >&2; exit 2 ;;
esac
command -v cmake >/dev/null
xcrun --sdk "$platform" --show-sdk-path >/dev/null
archive="$sdk/deps/opus-1.5.2.tar.gz"
source_dir="$sdk/src/opus-1.5.2"
build_dir="$sdk/build/opus-$platform-$arch"
prefix="$sdk/install/$platform-$arch"
test -f "$archive"
mkdir -p "$sdk/src"
if test ! -f "$source_dir/CMakeLists.txt"; then
    tar -xzf "$archive" -C "$sdk/src"
fi
cmake -S "$source_dir" -B "$build_dir" \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT="$platform" \
    -DCMAKE_OSX_ARCHITECTURES="$arch" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DBUILD_SHARED_LIBS=OFF \
    -DOPUS_BUILD_TESTING=OFF \
    -DOPUS_BUILD_PROGRAMS=OFF
cmake --build "$build_dir" --parallel 2
cmake --install "$build_dir"
