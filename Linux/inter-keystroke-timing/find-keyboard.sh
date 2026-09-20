#!/usr/bin/env bash
# Finds the /dev/input/eventN of the actual keyboard, not e.g. the power
# button (which also registers a "kbd" handler for its single key).
set -euo pipefail
cd "$(dirname "$0")"

DEV=$(awk '
/^N: Name=/ { name = $0 }
/^H: Handlers=/ {
    if (name ~ /[Kk]eyboard/ && $0 ~ /kbd/) {
        match($0, /event[0-9]+/)
        print substr($0, RSTART, RLENGTH)
        exit
    }
}' /proc/bus/input/devices)

if [[ -z "$DEV" ]]; then
    echo "couldn't auto-detect a keyboard device, check /proc/bus/input/devices" >&2
    echo "yourself and run ./build/keystroke-notify /dev/input eventN directly." >&2
    exit 1
fi

echo "keyboard device: $DEV" >&2

echo "proof we can't read it directly, as $(whoami):" >&2
cat "/dev/input/$DEV" 2>&1 >/dev/null | head -1 >&2 || true

exec ./build/keystroke-notify /dev/input "$DEV" "${1:-0.13}"
