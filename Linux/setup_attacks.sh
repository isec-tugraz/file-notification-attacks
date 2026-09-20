#!/usr/bin/env bash
# Run once as root, first thing after logging into this VM:
#   sudo bash ~/Linux-File-Notification-Attacks/setup_attacks.sh
#
# Creates the unprivileged spyuser account these PoCs run as (not in sudo,
# no hardware group membership), gives it its own copy of the artifacts
# (spyuser can't read into user's 700 home directory otherwise), and makes
# every script executable in both copies. auth-ui-redress stays user-only,
# see its own README for why.
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "run this as root: sudo bash ~/Linux-File-Notification-Attacks/setup_attacks.sh" >&2
    exit 1
fi

SRC_USER=$(logname 2>/dev/null || echo user)
SRC_DIR="/home/$SRC_USER/Linux-File-Notification-Attacks"
DST_DIR="/home/spyuser/Linux-File-Notification-Attacks"

if [[ ! -d "$SRC_DIR" ]]; then
    echo "$SRC_DIR not found." >&2
    exit 1
fi

if ! id spyuser &>/dev/null; then
    useradd -m -s /bin/bash spyuser
    echo "spyuser:password" | chpasswd
fi

rm -rf "$DST_DIR"
cp -r "$SRC_DIR" "$DST_DIR"
chown -R spyuser:spyuser "$DST_DIR"

chmod +x "$SRC_DIR"/*/*.sh "$SRC_DIR"/setup_attacks.sh "$SRC_DIR"/cleanup.sh 2>/dev/null
chmod +x "$DST_DIR"/*/*.sh "$DST_DIR"/setup_attacks.sh "$DST_DIR"/cleanup.sh 2>/dev/null

echo "setup done. spyuser ready, not in sudoers:"
id spyuser
