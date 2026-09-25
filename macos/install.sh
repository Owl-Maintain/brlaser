#!/bin/sh
# Install brlaser on macOS (Apple Silicon or Intel).
#
# CMake's "make install" puts the filter into /usr/libexec/cups, which macOS protects with
# SIP, and macOS's cupsd has no filter search path for third-party locations. This script
# installs the filter under /Library/Printers/brlaser and rewrites the PPDs accordingly.
#
# Usage:  sudo ./install.sh [path/to/rastertobrlaser] [path/to/ppd-dir]
#         (defaults: ./rastertobrlaser and ./ppd next to this script)
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
FILTER_SRC="${1:-$HERE/rastertobrlaser}"
PPD_SRC="${2:-$HERE/ppd}"
DEST=/Library/Printers/brlaser
PPD_DEST=/Library/Printers/PPDs/Contents/Resources

[ "$(id -u)" = 0 ] || { echo "Run with sudo: sudo $0"; exit 1; }
[ -x "$FILTER_SRC" ] || { echo "filter not found: $FILTER_SRC"; exit 1; }
[ -d "$PPD_SRC" ] || { echo "PPD directory not found: $PPD_SRC"; exit 1; }

mkdir -p "$DEST" "$PPD_DEST"
cp "$FILTER_SRC" "$DEST/rastertobrlaser"
chmod 755 "$DEST/rastertobrlaser"
# A binary unpacked from a downloaded zip inherits com.apple.quarantine; Gatekeeper then kills the
# filter when CUPS starts it (the job reports completed, nothing prints). CUPS itself doesn't check it.
xattr -d com.apple.quarantine "$DEST/rastertobrlaser" 2>/dev/null || true

n=0
for f in "$PPD_SRC"/*.ppd; do
  name=$(basename "$f")
  # 1. absolute filter path
  sed 's|\*cupsFilter: "application/vnd.cups-raster 33 rastertobrlaser"|*cupsFilter: "application/vnd.cups-raster 33 /Library/Printers/brlaser/rastertobrlaser"|' "$f" > "$PPD_DEST/$name"
  # 2. DCP/MFC models have a scanner: this key makes System Settings show "Open Scanner"
  #    next to the queue (scanning itself uses Brother's ICA driver or Image Capture).
  if grep -q -E '^\*ModelName: "Brother (DCP|MFC)' "$PPD_DEST/$name"; then
    sed -i '' 's|^\*cupsFilter:.*|&\
*APICADriver: True|' "$PPD_DEST/$name"
  fi
  xattr -d com.apple.quarantine "$PPD_DEST/$name" 2>/dev/null || true
  n=$((n+1))
done
chown -R root:wheel "$DEST"

# The filter prints its usage and exits 1 when run without arguments; exit 137 means macOS killed it.
rc=0; out=$("$DEST/rastertobrlaser" 2>&1) || rc=$?
case "$out" in
  *"rastertobrlaser job-id"*) ;;
  *) echo "ERROR: filter does not run on this Mac (exit $rc)"; exit 1 ;;
esac
echo "Installed rastertobrlaser and $n PPDs."
echo "Add the printer in System Settings > Printers & Scanners > Add Printer..., then in the"
echo "'Use' menu choose 'Select Software...' and search for 'brlaser'."
echo "(Add it through System Settings rather than lpadmin so the scanner button is linked.)"
