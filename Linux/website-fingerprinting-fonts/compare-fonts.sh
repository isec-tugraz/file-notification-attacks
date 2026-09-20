#!/usr/bin/env bash
# Live A/B website fingerprinting: font-spy keeps running the whole time,
# you load two different pages in the SAME already-open Firefox, and we
# slice its output at each checkpoint (line count, not time, since font-spy
# doesn't timestamp) to diff what each page touched.
#
# Caveat: this is a long-lived Firefox process, so a font already opened
# for site A won't show up again for site B even if site B also uses it,
# "only in B" means "fonts B needed that A hadn't already triggered", not
# "every font B uses". That's still a fingerprint, just a differential one.
# Firefox must already be open with no tabs/websites loaded before this
# script starts, so site A's numbers reflect fonts loaded fresh, not fonts
# a prior tab already had mmap'd.
set -euo pipefail
cd "$(dirname "$0")"
make -s

FONT_RE='\.(ttf|otf|ttc|pfb|woff2?)$'
LOG=$(mktemp)
./font-spy > "$LOG" 2>/dev/null &
SPY_PID=$!
trap 'kill "$SPY_PID" 2>/dev/null' EXIT

sleep 1
echo "font-spy is running against the already-open Firefox."
read -rp "Visit SITE A and wait for it to load, then open a new tab, close the tab with SITE A, and press ENTER when done... "
N1=$(wc -l < "$LOG")

read -rp "Visit SITE B and wait for it to load, then open a new tab, close the tab with SITE B, and press ENTER when done... "
N2=$(wc -l < "$LOG")

kill "$SPY_PID" 2>/dev/null
trap - EXIT

site_a=$(sed -n "1,${N1}p" "$LOG" | grep -E "$FONT_RE" | sort -u)
site_b=$(sed -n "$((N1 + 1)),${N2}p" "$LOG" | grep -E "$FONT_RE" | sort -u)

echo
echo "=== only site A touched ==="
comm -23 <(echo "$site_a") <(echo "$site_b")
echo
echo "=== only site B touched ==="
comm -13 <(echo "$site_a") <(echo "$site_b")
echo
echo "=== touched by both ==="
comm -12 <(echo "$site_a") <(echo "$site_b")

rm -f "$LOG"
