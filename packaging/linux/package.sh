#!/usr/bin/env bash
set -euo pipefail
BUILD="$(cd "$1" && pwd)"
OUT="$(mkdir -p "$2" && cd "$2" && pwd)"
VER="$3"
VST="$BUILD/ArSonComp_artefacts/Release/VST3/ArSonComp.vst3"
APP="$BUILD/ArSonComp_artefacts/Release/Standalone/ArSonComp"
test -d "$VST" && test -f "$APP"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
STAGE="$WORK/stage/ArSonComp"
mkdir -p "$STAGE/VST3" "$STAGE/Standalone"
cp -a "$VST" "$STAGE/VST3/"
install -m 755 "$APP" "$STAGE/Standalone/ArSonComp"
cp packaging/linux/install.sh packaging/linux/uninstall.sh "$STAGE/"
chmod 755 "$STAGE"/*.sh
cp LICENSE "$STAGE/LICENSE"
cp docs/INSTALL-PUBLIC.md "$STAGE/START-HERE.md"
tar -C "$WORK/stage" -czf "$OUT/ArSonComp-$VER-Linux-x86_64-Portable.tar.gz" ArSonComp
PKG="$WORK/deb"
mkdir -p "$PKG/DEBIAN" "$PKG/usr/lib/vst3" "$PKG/opt/arsoncomp" \
         "$PKG/usr/bin" "$PKG/usr/share/applications" "$PKG/usr/share/doc/arsoncomp"
cp -a "$VST" "$PKG/usr/lib/vst3/"
install -m 755 "$APP" "$PKG/opt/arsoncomp/ArSonComp"
cat > "$PKG/usr/bin/arsoncomp" <<'EOF'
#!/bin/sh
exec /opt/arsoncomp/ArSonComp "$@"
EOF
chmod 755 "$PKG/usr/bin/arsoncomp"
cat > "$PKG/usr/share/applications/arsoncomp.desktop" <<'EOF'
[Desktop Entry]
Name=ArSonComp
Comment=Real-time audio dynamics compressor
Exec=arsoncomp
Type=Application
Categories=AudioVideo;Audio;
Terminal=false
EOF
cp LICENSE "$PKG/usr/share/doc/arsoncomp/LICENSE"
cp docs/INSTALL-PUBLIC.md "$PKG/usr/share/doc/arsoncomp/START-HERE.md"
cat > "$PKG/DEBIAN/control" <<EOF
Package: arsoncomp
Version: $VER
Section: sound
Priority: optional
Architecture: amd64
Maintainer: ArSon Audio <noreply@github.com>
Description: ArSonComp real-time compressor VST3 and Standalone
 Native x86_64 VST3 plug-in and desktop application.
EOF
dpkg-deb --root-owner-group --build "$PKG" "$OUT/ArSonComp-$VER-Ubuntu-Debian-amd64.deb"
echo "Linux packages ready: $OUT"
