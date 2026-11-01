# ArSonComp — simple public installers

Choose the file matching your operating system on
https://github.com/masarray/arson-comp/releases.
You do NOT need to move individual VST3 DLLs or browse random plug-in paths.

## Windows 10/11 x64
Double-click **ArSonComp-...-Windows-x64-Setup.exe**.
Default: install VST3 + Standalone. Choose **current user** to avoid UAC
or **all users** if you want every Windows account to use ArSonComp.
The guided installer uses Steinberg's official Windows VST3 paths,
registers an uninstaller, and adds a Start Menu shortcut for Standalone.
Restart or rescan your DAW. If you installed ArSonComp in the *other*
scope previously, remove that duplicate; the installer warns about it.

## Ubuntu/Debian x86_64
Double-click **ArSonComp-...-Ubuntu-Debian-amd64.deb** and confirm in
Software Center / App Center. A terminal alternative is:
  sudo apt install ./ArSonComp-*-Ubuntu-Debian-amd64.deb
This installs VST3 in /usr/lib/vst3 and the application in your launcher.
Uninstall using your software manager or: sudo apt remove arsoncomp.

## Other x86_64 Linux
Extract **Linux-x86_64-Portable.tar.gz**. In its ArSonComp folder
run **bash install.sh**. No sudo is required; the VST3 is placed in ~/.vst3
and the Standalone gets an application-menu entry.
Run **bash uninstall.sh** from the extracted folder to uninstall.
This targets modern glibc-based Linux distributions, not musl/Alpine.

## macOS 11+ — Apple Silicon and Intel
Open **macOS-Universal-AdHoc.pkg**. The macOS Installer places VST3 in
/Library/Audio/Plug-Ins/VST3 and the Standalone in /Applications.
The VST3 and app use **ad-hoc signatures**, and this PKG is unsigned and
NOT Apple notarized. macOS Gatekeeper can warn or block installation.
For a trusted official download only, Finder > right click > Open or
System Settings > Privacy & Security > Open Anyway may allow it.
Do not disable Gatekeeper globally. A Developer ID certificate and Apple
notarization are needed for a truly frictionless public Mac installer.
A VST3-only universal ZIP is provided as a fallback for experienced users.

## First use on any OS
Restart/rescan your VST3 host. Find ArSonComp under dynamics/compressor.
For Standalone, choose the actual audio input/output in Audio Settings.
Start at 48 kHz / 128 samples, low headphone volume, and use a
reasonable Clean vocal setting before lowering the buffer.
The plugin's zero *algorithmic* sample delay is not zero driver or
hardware-monitoring round-trip latency.

## Reliability and distribution notes
Automated CI proves successful builds and in-process tests only, not
physical interface reliability, multi-host certification, or sonic
parity with commercial compressors. Use Windows START-HERE and the
M7 field-test checklist for the Maono PS22 / ASIO hardware gate.
For open-source distribution, keep AGPL source accessible and review
JUCE AGPL/commercial terms plus Steinberg ASIO SDK redistribution
requirements remain important even for an owner-approved Stable release. The user-confirmed physical test is not a substitute for independent licensing review.

Official VST3 standard installation locations:
https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/Locations%2BFormat/Plugin%2BLocations.html
