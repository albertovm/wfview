#!/bin/sh
# Build for a device (default) or simulator; device signing is a separate step.
set -eu
project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk=${WFVIEW_QT_ROOT:-/Volumes/1TbSilver/wfview-ios}
platform=${WFVIEW_IOS_PLATFORM:-iphoneos}
case "$platform" in
    iphoneos) build_name=wfview-device; destination='generic/platform=iOS'; arch=arm64 ;;
    iphonesimulator) build_name=wfview-simulator; destination='generic/platform=iOS Simulator'; arch=${WFVIEW_SIM_ARCH:-x86_64} ;;
    *) echo 'WFVIEW_IOS_PLATFORM must be iphoneos or iphonesimulator' >&2; exit 2 ;;
esac
# Generate before starting Xcode: regenerating inside a build can leave Xcode
# using the previous build settings for that invocation.
sh "$project_root/scripts/configure-ios.sh" CONFIG+=sdk_no_version_check
xcodebuild -project "$sdk/build/$build_name/wfview.xcodeproj" \
    -scheme wfview -configuration Release -sdk "$platform" \
    -destination "$destination" ARCHS="$arch" ONLY_ACTIVE_ARCH=YES \
    CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO build "$@"
app="$sdk/build/$build_name/Release-$platform/wfview.app"
sh "$project_root/scripts/compile-ios-app-icon.sh" "$app" "$platform"
