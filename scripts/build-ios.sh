#!/bin/sh
# Build an unsigned arm64 app for iPhone and iPad; signing is a separate step.
set -eu
project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk=${WFVIEW_QT_ROOT:-/Volumes/1TbSilver/wfview-ios}
# Generate before starting Xcode: regenerating inside a build can leave Xcode
# using the previous build settings for that invocation.
sh "$project_root/scripts/configure-ios.sh" CONFIG+=sdk_no_version_check
exec xcodebuild -project "$sdk/build/wfview-device/wfview.xcodeproj" \
    -scheme wfview -configuration Release -sdk iphoneos \
    -destination 'generic/platform=iOS' \
    CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO build "$@"
