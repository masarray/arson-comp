#!/usr/bin/env bash
set -euo pipefail
DEST="$HOME/.vst3/ArSonComp.vst3"
APPDIR="$HOME/.local/opt/arsoncomp"
if [[ -L "$DEST" || -L "$APPDIR" ]]; then
  echo "Refusing to follow symlinks." >&2; exit 2
fi
rm -rf -- "$DEST" "$APPDIR"
rm -f -- "$HOME/.local/bin/arsoncomp" "$HOME/.local/share/applications/arsoncomp.desktop"
echo "Removed ArSonComp for the current user. Presets and backups preserved."
