#!/bin/sh
# Remove brlaser from macOS: print queues that use it, the PPDs and the filter.
# Usage:  sudo ./uninstall.sh
[ "$(id -u)" = 0 ] || { echo "Run with sudo: sudo $0"; exit 1; }
FILTER=/Library/Printers/brlaser/rastertobrlaser
for q in $(lpstat -p 2>/dev/null | awk '{print $2}'); do
  grep -qs "$FILTER" "/etc/cups/ppd/$q.ppd" && lpadmin -x "$q" && echo "removed queue $q"
done
n=0
for f in /Library/Printers/PPDs/Contents/Resources/*.ppd; do
  grep -qs "$FILTER" "$f" && rm -f "$f" && n=$((n+1))
done
rm -rf /Library/Printers/brlaser
echo "Removed the filter and $n PPDs."
