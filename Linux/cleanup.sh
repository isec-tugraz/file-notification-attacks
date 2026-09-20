#!/usr/bin/env bash
# Run as root inside linux-vm (sudo bash cleanup.sh) between runs of these
# PoCs: kills any leftover attack processes, drops spyuser, wipes shell
# history and Firefox state, and removes build artifacts/generated test
# data so the next run starts from the same state as a fresh checkout.
set -uo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "run this as root (sudo bash cleanup.sh)" >&2
    exit 1
fi

for p in font-spy keystroke-notify inotifywait window-launcher \
         inotify-watcher-with-gui watcher accessor firefox firefox-esr; do
    pkill -9 -f "$p" 2>/dev/null
done
sleep 1

rm -f /tmp/*.log

# kill spyuser's whole login session (systemd --user, pipewire, sshd, etc.)
# before userdel, otherwise it fails with "user is currently used by process"
loginctl terminate-user spyuser 2>/dev/null
pkill -9 -u spyuser 2>/dev/null
sleep 1
userdel -r spyuser 2>/dev/null

rm -f /home/user/.bash_history /home/user/.zsh_history
rm -f /root/.bash_history /root/.zsh_history

rm -rf /home/user/.mozilla

# whole KDE/GTK cache tree, entirely regenerated on next login, but also
# where clipboard history, recent-file lists, and the search index (baloo)
# quietly accumulate real testing data
rm -rf /home/user/.cache/*
rm -rf /home/user/.local/share/klipper /home/user/.local/share/baloo \
       /home/user/.local/share/akonadi
rm -f /home/user/.local/share/recently-used.xbel \
      /home/user/.local/share/user-places.xbel* \
      /home/user/.local/share/RecentDocuments/*.desktop

# login/session history and system logs, in case anyone diffs the artifact
# against a stock Debian install and wonders who logged in when
: > /var/log/wtmp
: > /var/log/btmp
: > /var/log/lastlog
rm -f /var/log/Xorg.0.log /var/log/Xorg.0.log.old \
      /var/log/boot.log /var/log/boot.log.1 \
      /var/log/dpkg.log /var/log/apt/*.log
journalctl --rotate 2>/dev/null
journalctl --vacuum-time=1s 2>/dev/null

find /home/user/Linux-File-Notification-Attacks -maxdepth 2 -type d -name build -exec rm -rf {} + 2>/dev/null
rm -f /home/user/Linux-File-Notification-Attacks/website-fingerprinting-fonts/font-spy
rm -f /home/user/Linux-File-Notification-Attacks/auth-ui-redress/window-launcher \
      /home/user/Linux-File-Notification-Attacks/auth-ui-redress/inotify-watcher-with-gui
rm -f /home/user/Linux-File-Notification-Attacks/temporal-resolution/testing_file \
      /home/user/Linux-File-Notification-Attacks/temporal-resolution/op-watch \
      /home/user/Linux-File-Notification-Attacks/temporal-resolution/op-accs

echo "cleanup done."
id spyuser 2>&1
