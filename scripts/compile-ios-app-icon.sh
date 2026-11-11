#!/bin/sh
# Compile the iOS asset catalog and register its AppIcon metadata in the app.
set -eu
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <app-bundle> <iphoneos|iphonesimulator>" >&2
    exit 2
fi
app=$1
platform=$2
case "$platform" in
    iphoneos|iphonesimulator) ;;
    *) echo "Unsupported Apple platform: $platform" >&2; exit 2 ;;
esac
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
xcrun actool --compile "$tmp" \
    --platform "$platform" \
    --minimum-deployment-target 16.0 \
    --app-icon AppIcon \
    --output-partial-info-plist "$tmp/icon-info.plist" \
    "$root/platform/ios/Assets.xcassets"
cp "$tmp/Assets.car" "$app/Assets.car"
python3 - "$app/Info.plist" "$tmp/icon-info.plist" <<'PY'
import plistlib
import sys
from pathlib import Path

app_info, icon_info = map(Path, sys.argv[1:])
with app_info.open('rb') as f:
    app = plistlib.load(f)
with icon_info.open('rb') as f:
    app.update(plistlib.load(f))
with app_info.open('wb') as f:
    plistlib.dump(app, f, fmt=plistlib.FMT_XML, sort_keys=False)
PY
