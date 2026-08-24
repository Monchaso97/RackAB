#!/usr/bin/env bash
# ============================================================
#  RackAB - macOS .dmg builder  (RUN THIS ON A MAC)
#
#  Windows cannot compile a macOS VST3, so this builds it on a Mac and
#  wraps it in a drag-to-folder .dmg. No installer wizard, no standalone.
#
#  Result: RackAB.dmg -> the user opens it and drags RackAB.vst3 onto the
#  "VST3 Plugins" shortcut. Works in FL Studio, Pro Tools, etc.
#
#  NOTE: without an Apple Developer ID the plugin is NOT notarized, so the
#  first time the user must right-click RackAB.vst3 > Open (or run the
#  command in "READ ME FIRST.txt") once to clear Gatekeeper's quarantine.
#
#  Requirements: Xcode command line tools, CMake, JUCE 9.x.
#  Usage:  ./build_dmg.sh [/path/to/JUCE]
# ============================================================
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$HERE/../.." && pwd)"
JUCE_PATH="${1:-$HOME/JUCE}"
VERSION="1.0.0"
BUILD_DIR="$PROJECT_ROOT/build-mac"
STAGE="$HERE/dmg-stage"
OUT="$HERE/output"

echo "==> Configuring (JUCE at: $JUCE_PATH)"
cmake -B "$BUILD_DIR" -S "$PROJECT_ROOT" -G Xcode \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DJUCE_PATH="$JUCE_PATH"

echo "==> Building Release (VST3 only, no standalone)"
cmake --build "$BUILD_DIR" --config Release --target RackAB_VST3

VST3="$BUILD_DIR/RackAB_artefacts/Release/VST3/RackAB.vst3"
[ -d "$VST3" ] || { echo "ERROR: build did not produce $VST3" >&2; exit 1; }

# Ad-hoc sign so the bundle is at least self-consistent (does NOT bypass
# Gatekeeper notarization; that needs a paid Developer ID).
echo "==> Ad-hoc signing"
codesign --force --deep --sign - "$VST3" || true

echo "==> Staging .dmg contents"
rm -rf "$STAGE" "$OUT"
mkdir -p "$STAGE" "$OUT"
cp -R "$VST3" "$STAGE/"
# Shortcut to the system VST3 folder so the user just drags onto it.
ln -s "/Library/Audio/Plug-Ins/VST3" "$STAGE/VST3 Plugins"
cp "$HERE/resources/READ ME FIRST.txt" "$STAGE/READ ME FIRST.txt"
[ -f "$HERE/resources/welcome.png" ] && cp "$HERE/resources/welcome.png" "$STAGE/.background.png"

echo "==> Creating .dmg"
hdiutil create -volname "RackAB" \
    -srcfolder "$STAGE" \
    -ov -format UDZO \
    "$OUT/RackAB-$VERSION-macOS.dmg"

rm -rf "$STAGE"
echo "==> Done: $OUT/RackAB-$VERSION-macOS.dmg"
