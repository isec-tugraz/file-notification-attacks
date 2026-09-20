# File Notification Attacks - Artifacts
This repository contains the (still in review!) artifacts for the paper "File
Notification Attacks: Templating and Exploiting Side-Channel Leakage from the
File-Notification Systems on Linux, Windows, and macOS", accepted at [CCS
'26](https://www.sigsac.org/ccs/CCS2026/).

Visit the website with demos at: https://inoti.fyi/

Read the paper at: https://snee.la/pdf/pubs/file-notification-attacks.pdf or
http://inoti.fyi/pubs/file-notification-attacks.pdf

### Table Of Contents
* [ Foreword ](#foreword)
* [ Linux ](#linux)
    * [ Requirements: Kernel Version ](#requirements-kernel-version)
    * [ Setting Up ](#setting-up)
    * [ Attacks ](#attacks)
        * [ 1. Unreadable File Bypass ](#1-unreadable-file-bypass)
        * [ 2. Temporal Resolution ](#2-temporal-resolution)
        * [ 3. Inter-Keystroke Timing ](#3-inter-keystroke-timing)
        * [ 4. Auth UI Redress ](#4-auth-ui-redress)
        * [ 5. Website Fingerprinting via Fonts ](#5-website-fingerprinting-via-fonts)
* [ Android ](#android)
* [ Windows ](#windows)
    * [ Hardware / Software Requirements ](#hardware--software-requirements-1)
    * [ Direct Website Leakage on Firefox ](#direct-website-leakage-on-firefox)
* [ macOS ](#macos)
    * [ Hardware / Software Requirements ](#hardware--software-requirements-2)
    * [ Watching /Applications ](#watching-applications)

## Foreword
As a part of [artifact evaluation at
CCS'26](https://www.sigsac.org/ccs/CCS2026/call-for/call-for-artifacts.html)
(still ongoing!), we shipped a Debian 13 + KDE Plasma 6 VM with a pre-patched
kernel. This VM was not used to produce the paper's evaluation results and is
not intended to reproduce their exact magnitude. This repository may be updated
to incorporate reviews and feedback from our artifact evaluators. We will make
the VM publicly accessible when our artifact evaluation is completed.

We employed Claude (Opus 4.8) for wrapping our code into decently
well-documented and high-performant artifacts (for example, the Windows one was
a bit laggy). However, the code, scripts, makefiles it generated were based on

The instructions for testing Linux code below is specific to running commands in
the VM. Please look at the code instead in `Linux/`.

The VM exists purely for convenience, so that mounting each attack does not
require a from-scratch environment setup or kernel downgrade. The underlying
mechanism demonstrated in the VM is identical to the one described in the paper,
but please note that we did not use the VM to produce the numbers reported in
the paper. Absolute timings may differ due to the virtualized environment and
underlying hardware.

## Linux
Findings are reproduced inside a Debian 13 VM (`linux-vm/`), installed from the
official live Debian ISO with KDE Plasma 6 (Wayland) installed with a kernel
that predates the [fsnotify
fix](https://lore.kernel.org/linux-cve-announce/2026011303-CVE-2025-68788-05bd@gregkh/).
`inotify-tools`, Qt6 dev headers, and `pkexec` are installed. Nothing else is
upgraded, and the kernel is never touched after install. Please **DO NOT** run
`apt upgrade` or attempt to upgrade the kernel. This VM image is
**deliberately** old, is not up to date, and does not have the latest security
patches. This VM image is only meant for quick validation and testing!

The code, attacks, and demonstrations located within the VM are mirrored in
`Linux/`.

**Note**: We mention as which user to run the commands at the top of each
command block. There is the `[host]`, which is the host machine which runs the
VM, `[user]` which is the victim of the attacks in the VM, and `[spyuser]` which
requires to be set up and will be the attacker in most Linux attacks.

The disk image is shipped as `linux-vm-upload.qcow2`, compressed with zstd.
Rename it to `linux-vm.qcow2` before running `run.sh` (which expects that
filename):

```sh
# Run As: [host]
cd linux-vm/
mv linux-vm-upload.qcow2 linux-vm.qcow2
./run.sh
```

Reading zstd-compressed qcow2 requires `qemu-img`/`qemu-system-x86_64` version
5.2 or newer (2020+), check with `qemu-img --version`. If you're stuck on an
older QEMU and it fails to open the image, decompress it into a flat qcow2
first:

```sh
qemu-img convert -O qcow2 linux-vm-upload.qcow2 linux-vm.qcow2
```

4 cores, 4GB RAM, GUI display. Logins are:
- username `root`, password `password`,
- username `user`, password `password`,
- username `spyuser`, password `password` (do not login as `spyuser`!)

### Setting Up
The supplied VM already runs a vulnerable, pre-patched kernel
(`6.12.43+deb13-amd64`), so no downgrade is needed to use it. You will need QEMU
to run this image (which has a GUI). The artifact lives at
`/home/user/Linux-File-Notification-Attacks` inside the VM.

One command sets things up fresh in the VM: (i) an unprivileged `spyuser`
account (non-sudo), (ii) its own copy of the artifact directory (a separate
unprivileged account cannot traverse into `user`'s home directory otherwise),
(iii) and every script made executable in both copies.

```sh
# In the VM, Run As: [user]
sudo bash ~/Linux-File-Notification-Attacks/setup_attacks.sh
```

Every attack below runs as `spyuser` (`su - spyuser`, password `password`),
except `auth-ui-redress`, which runs as `user` (explained below).

### Attacks

#### 1. Unreadable File Bypass
Proves file-operation notifications are delivered on files that cannot be
read directly, as long as their parent directory is readable.

```sh
# Run As: [spyuser]
# To switch to spyuser, in a new terminal type `su - spyuser`. The password
# is `password`.
cd ~/Linux-File-Notification-Attacks/unreadable-file-bypass
./watch-syslog.sh
```

Leave it running, then generate a log line from another terminal as `user`:

```sh
# Run As: [user]
logger "hello"
```

This Debian 13 image has no rsyslog, so there is no `/var/log/syslog` file as
shown in the paper. The script falls back to watching
`/var/log/journal/<machine-id>/` instead. This is the same idea, as the
`.journal` files being monitored are owned by `root` (user) and
`systemd-journal` (group), neither of which are `spyuser`.


#### 2. Temporal Resolution
Measures inotify's notification delay.

```sh
# Run As: [user], ensure numpy is installed (or pip3)
sudo apt install python3-numpy
```

```sh
# Run As: [spyuser]
cd ~/Linux-File-Notification-Attacks/temporal-resolution
./run.sh
```

`watcher` opens an inotify watch on a test file and timestamps every IN_ACCESS
it receives. `accessor` reads that same file 1000 times with a random 5-10ms
delay between reads, timestamping each read itself. `stats.py` diffs the two
timestamp recordings and prints the average/stddev/min delay between the read
happening and the notification arriving, *i.e.* the temporal resolution of
inotify.


#### 3. Inter-Keystroke Timing
Not the full attack from the paper, but just the filtering proof-of-concept
primitive that the attack is built on: for every key pressed anywhere on the
system, print one timestamped notification, without ever reading `/dev/input`
directly.

```sh
# Run As: [spyuser]
cd ~/Linux-File-Notification-Attacks/inter-keystroke-timing
make
./find-keyboard.sh
```
Type in any window (maybe a new terminal as `user`). Each keystroke prints one
`KEYPRESS` line. The suppression window (which we heuristically chose for this
demonstration is 130ms) merges several IN_ACCESS events generated by a single
physical key press. In practice, we have noticed that this window is different
for different hardware (e.g., mechanical keyboards, typing styles).

If auto-detection picks the wrong device (or none), check
`/proc/bus/input/devices` and run it directly with `./build/keystroke-notify
/dev/input event4`.

#### 4. Auth UI Redress
We demonstrate being able to detect when
(`pkexec`)[https://polkit.pages.freedesktop.org/polkit/] is executed, and
further draw a fake window atop it. As we showed in Section 4.4.3, this
'authentication UI redress attack' is possible on KDE Plasma 5 and 6 with
Wayland. We execute this attack in the same-user attacker threat model: Wayland
socket is scoped to the logged-in session, and a separate unprivileged account
can't draw on it.

On a stock KDE install `pkexec` is pulled in by the desktop, but this live ISO
is a leaner image, and thus does not have it installed. Therefore, we installed
it manually while (trying to) keep the VM img small.

```sh
# Run As: [user]
cd ~/Linux-File-Notification-Attacks/auth-ui-redress
make
./inotify-watcher-with-gui
```
Leave it running in a terminal. From another terminal (directory does not
matter), trigger a real `pkexec` prompt, *e.g.* `pkexec ls`. The watcher sees
the access to `/usr/bin/pkexec` and draws a fake "Authentication Required"
dialog (`window-launcher`) onto the screen atop the real one. Typing into the
fake dialog and hitting Authenticate prints the entered text to the watcher's
terminal, then closes it. Please note that the fake window is deliberately
different from the real window for easy comparison.

#### 5. Website Fingerprinting via Fonts
Watching font directories while a page loads reveals which font files it
touched. Different sites pull in different fonts, so the set of paths
printed while a page loads is already a fingerprint, no timing or
classifier needed for this *minimal* version.

First, as user, open Firefox (click it via the icon in the task manager dock at
the bottom of the screen), and make sure that no websites are open. Then, in a
terminal as `spyuser`:

```sh
# Run As: [spyuser]
cd ~/Linux-File-Notification-Attacks/website-fingerprinting-fonts
./compare-fonts.sh
```

This builds `font-spy` which starts monitoring the font directories used by
Firefox. The compare-fonts script prompts you to visit two websites.

First, visit one website (perhaps wikipedia.com) and wait a few seconds for it
to load, open a new tab, close the old tab (with the website), and then hit
ENTER in the terminal. 

Second, do the same with another website (perhaps reddit.com): visit the website
and wait for a few seconds for it to load, open a new tab, close the old tab,
and then hit ENTER in the terminal.

The script should print the difference in fonts accessed per website. Note that
in the paper, we also used the temporal information, *i.e.*, when the font was
accessed. For a simple proof-of-concept, we disregard this information and just
simply print the difference between the two sets of font accesses. While the
exact set of font accesses may differ between runs, there are certain font files
that are always and uniquely accessed by a website.

Note that websites may change over time, and thus, the font file accesses may
differ. At the time of drafting this artifact, we notice that Wikipedia always
accesses `/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf` and
Reddit always accesses
`/usr/share/fonts/truetype/vlgothic/VL-Gothic-Regular.ttf`.


## Android

### Hardware / Software Requirements
A physical device or emulator running an Android version whose Scoped Storage
model still permits registering a `FileObserver` (the Java-level wrapper around
`inotify`) on WhatsApp's shared media directories, and a second device/account
to act as the message sender. Either build the provided source
(`Android/source/`) or install the supplied APK directly
(`Android/app-release.apk`).

### Revealing Private Communication on WhatsApp
Android exposes the same `inotify` primitive used throughout the Linux
findings via `android.os.FileObserver`. Section 5.4.3 shows that an
unprivileged, permission-less app can register such an observer on
WhatsApp's media directories and, purely from the resulting stream of
open/close/access events, infer *that* private media was received, without
holding any permission that would let it read the content itself.

Launch the app and trigger the refresh action; the observer service attaches
and begins emitting periodic liveness entries. Scroll to the bottom of the
log view and continue refreshing until repeated `ObserverService: Still
Running` entries appear, confirming the watch is active and stable.

From a second account, send an image or document to the observed
conversation, and, if not already cached, download it on the observed
device. This produces a burst of file-event log lines; immediately following
the last `ObserverService: Still Running` entry before the burst, the log
should show open/close/access events whose paths correspond to the received
file, demonstrating that its arrival is observable purely from file-system
notification metadata.

## Windows
We provide the source code which can be compiled with msys2. Otherwise, the exe
file (Windows/firefox-fingerprint/monitor.exe) can also be used.

### Hardware / Software Requirements
A Windows machine (or VM) with [MSYS2](https://www.msys2.org/) installed, and
the MinGW-w64 `g++` toolchain (`pacman -S mingw-w64-ucrt-x86_64-gcc`, run from
an MSYS2 shell). Firefox installed.

### Direct Website Leakage on Firefox
This proof of concept uses
[`ReadDirectoryChangesW`](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readdirectorychangesw)
to recursively watch all of `C:\`, and only prints events whose path contains
`http` (per-origin storage directories that Firefox names after the site's
scheme/host, *e.g.* under its cache/IndexedDB folders). No admin rights are
required to watch a directory you can list. This per-origin storage is written
to on every page load, so watching from outside the browser process still
reveals which site was just visited.

Either use the exe we provide (Windows/firefox-fingerprint/monitor.exe) or
compile from an MSYS2 UCRT64 shell:

```sh
# Run in an MSYS2 UCRT64 shell
cd Windows/firefox-fingerprint
g++ -municode -static -O2 -o monitor.exe monitor.cpp
```

Use `g++`, not `gcc`. Also run `monitor.exe` from a terminal (not by
double-clicking it), otherwise there is no console attached to print to.

The watcher runs as a second, unprivileged local account, separate from the one
browsing. First, create that account via Settings:

1. Open **Settings > Accounts > Other Users** (Windows 11).
2. Click **Add Other User**.
3. Click **I don't have this person's sign-in information**.
4. Click **Add a user without a Microsoft account**.
5. Enter a username, *e.g.* `attacker`, and a password, then **Next**.

This creates a **local** account (not tied to a Microsoft account, no
network sign-in), standard (non-admin) privileges by default.

Next, put `monitor.exe` somewhere the new account can actually read and
execute it. Copy the built binary into the public, world-readable directory
instead:

```cmd
copy monitor.exe C:\Users\Public\monitor.exe
```

Now, from your main (victim) account, launch it under the `attacker`
account without switching desktops or logging out, using `runas`:

```cmd
runas /user:attacker "cmd /k C:\Users\Public\monitor.exe"
```

Enter `attacker`'s password when prompted. `cmd /k` (rather than running
`monitor.exe` directly) keeps the console window open so you can watch its
output, while simply `runas /user:attacker C:\Users\Public\monitor.exe` also
works, but its window closes the instant the process exits. This is purely for
convenience.

Leave the `monitor.exe` window running, then, back in your main account, open
Firefox and visit a couple of different websites, *e.g.*,
[arstechnica.com](https://arstechnica.com) and [reddit.com](https://reddit.com).
Each line printed is a file create/modify/rename under a storage path matching
`http`. Different sites get different origin directories, and thus the set of
paths touched while a page loads is already a fingerprint.

## macOS

### Hardware / Software Requirements
For simplicity, we use [`fswatch`](https://github.com/emcrisostomo/fswatch), an
existing, minimal, widely-used CLI tool that wraps the native FSEvents API. An
attacker can use the FSEvents API without this tool too.

### Watching /Applications
Install `fswatch`, or build from source:

```sh
brew install fswatch
```

Watch `/Applications` recursively, as an unprivileged user:

```sh
fswatch -xr /Applications
```

Leave it running, then, from the Finder or a browser, download and install Zoom
(or any other application), then uninstall it (you may have to empty trash too).
`fswatch` prints every path FSEvents reports under `/Applications` as it
happens. For quick evaluation, the same user can monitor the directory.
Otherwise, you can set up a new user and use fswatch as that user. 


## License
Released under the [MIT License](./LICENSE).
