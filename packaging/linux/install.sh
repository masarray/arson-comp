#!/usr/bin/env bash
# Safe current-user installer. No sudo, no host-specific custom folder.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
if [[ "$(uname -s)" != "Linux" ]]; then echo "Linux only" >&2; exit 2; fi
if [[ ! -d "$ROOT/VST3/ArSonComp.vst3" || ! -f "$ROOT/Standalone/ArSonComp" ]]; then
  echo "Incomplete archive: missing VST3 or Standalone" >&2; exit 3
fi
DEST="$HOME/.vst3/ArSonComp.vst3"
APPDIR="$HOME/.local/opt/arsoncomp"
BIN="$HOME/.local/bin/arsoncomp"
DESKTOP="$HOME/.local/share/applications/arsoncomp.desktop"
if [[ -L "$DEST" || -L "$APPDIR" ]]; then
  echo "Refusing to overwrite a symlinked install path." >&2; exit 4
fi
mkdir -p "$HOME/.vst3" "$APPDIR" "$(dirname "$BIN")" "$(dirname "$DESKTOP")"
if [[ -d "$DEST" ]]; then
  mv "$DEST" "$DEST.backup.$(date +%s)"
fi
cp -a "$ROOT/VST3/ArSonComp.vst3" "$DEST"
install -m 755 "$ROOT/Standalone/ArSonComp" "$APPDIR/ArSonComp"
cat > "$BIN" <<'EOF'
#!/bin/sh
exec "$HOME/.local/opt/arsoncomp/ArSonComp" "$@"
EOF
chmod 755 "$BIN"
printf '[Desktop Entry]\nName=ArSonComp\nComment=Real-time dynamics compressor\nExec="%s"\nType=Application\nCategories=AudioVideo;Audio;\nTerminal=false\n' "$BIN" > "$DESKTOP"
echo "Installed VST3: $DEST"
echo "Installed Standalone: $APPDIR/ArSonComp"
echo "Launch via application menu or $BIN; rescan/restart your DAW."
