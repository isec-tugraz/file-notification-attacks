#!/usr/bin/env bash
# /var/log/syslog isn't world-readable, but /var/log is. Watching the
# directory still delivers events for syslog, without ever reading it.
#
# Recent Debian/Ubuntu installs are journald-only (no rsyslog, no flat
# /var/log/syslog), so if that file isn't there we fall back to the journal:
# /var/log/journal/<machine-id>/ is world-readable+executable, but the
# .journal files inside are root:systemd-journal 640, unreadable to us. Same
# bypass, different file. journald splits writes across system.journal and
# one user-<uid>.journal per uid, so we watch for any *.journal write rather
# than pin one filename. `logger "hello"` still works as the trigger either
# way, journald accepts it over the same /dev/log socket rsyslog would.
set -euo pipefail

if [[ -e /var/log/syslog ]]; then
    DIR=/var/log
    TARGET=${1:-syslog}

    echo "proof we can't read it directly, as $(whoami):"
    cat "$DIR/$TARGET" 2>&1 | head -1 || true
    echo

    echo "watching $DIR for events on $TARGET ..."
    inotifywait -m "$DIR" -e access -e modify --format '%T %f %e' --timefmt '%H:%M:%S' \
        | grep --line-buffered -F "$TARGET"
else
    MID=$(ls /var/log/journal 2>/dev/null | head -1)
    DIR=/var/log/journal/${MID:-missing}

    if [[ -z "$MID" || ! -d "$DIR" ]]; then
        echo "no /var/log/syslog and no /var/log/journal/<machine-id> either." >&2
        ls -la /var/log >&2
        exit 1
    fi

    echo "proof we can't read the journal files directly, as $(whoami):"
    ls -la "$DIR"
    cat "$DIR"/system.journal 2>&1 | head -1 || true
    echo

    echo "watching $DIR for writes, run 'logger \"hello\"' from another terminal now ..."
    inotifywait -m "$DIR" -e modify --format '%T %f %e' --timefmt '%H:%M:%S' \
        | grep --line-buffered -F '.journal'
fi
