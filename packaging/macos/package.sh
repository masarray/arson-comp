#!/usr/bin/env bash
set -euo pipefail
BUILD="$(cd "$1" && pwd)"
OUT="$(mkdir -p "$2" && cd "$2" && pwd)"
VER="$3"
VST="$BUILD/ArSonComp_artefacts/Release/VST3/ArSonComp.vst3"
APP="$BUILD/ArSonComp_artefacts/Release/Standalone/ArSonComp.app"
test -d "$VST" && test -d "$APP"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
ROOT="$WORK/pkgroot"
mkdir -p "$ROOT/Library/Audio/Plug-Ins/VST3" "$ROOT/Applications"
ditto "$VST" "$ROOT/Library/Audio/Plug-Ins/VST3/ArSonComp.vst3"
ditto "$APP" "$ROOT/Applications/ArSonComp.app"
# Ad-hoc sign top-level bundles individually; no deep signing.
# Not Developer ID / notarized: Gatekeeper can still block public installs.
for bundle in "$ROOT/Library/Audio/Plug-Ins/VST3/ArSonComp.vst3" \
              "$ROOT/Applications/ArSonComp.app"; do
  codesign --force --sign - --timestamp=none "$bundle"
  codesign --verify --verbose=2 "$bundle"
done
BIN="$ROOT/Applications/ArSonComp.app/Contents/MacOS/ArSonComp"
test -f "$BIN"
ARCHS="$(lipo -archs "$BIN")"
echo "Mac architectures: $ARCHS"
case "$ARCHS" in
  *arm64*x86_64*|*x86_64*arm64*) ;;
  *) echo "Expected universal arm64 + x86_64 binary" >&2; exit 7 ;;
esac
pkgbuild --root "$ROOT" --install-location / \
  --identifier com.arsonaudio.comp.installer --version "$VER" \
  "$OUT/ArSonComp-$VER-macOS-Universal-AdHoc.pkg"
ditto -c -k --sequesterRsrc --keepParent \
  "$ROOT/Library/Audio/Plug-Ins/VST3/ArSonComp.vst3" \
  "$OUT/ArSonComp-$VER-macOS-VST3-Universal.zip"
echo "Unsigned installer / ad-hoc signed plugin and Standalone are packaged."
