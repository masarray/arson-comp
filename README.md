# ArSonComp

ArSonComp is a lightweight real-time dynamics compressor for vocals,
karaoke, streaming and music, written in C++17 with a reusable JUCE-free
audio engine and a JUCE 8 VST3/Standalone interface.

## Download for Windows, Linux and macOS

**[GitHub Releases](https://github.com/masarray/arson-comp/releases)**

- **Windows 10/11 x64:** Inno Setup EXE. Choose *Just me* (no admin) or
  *All users*. Installs VST3 and Standalone by default, Start Menu/uninstaller.
- **Ubuntu/Debian x86_64:** DEB installer with VST3 and Standalone.
- **Other desktop Linux x86_64:** portable archive with no-sudo
  per-user install/uninstall scripts.
- **macOS Apple Silicon + Intel:** universal VST3 and Standalone in a PKG.
  Ad-hoc code signed; unsigned/not-notarized PKG may trigger Gatekeeper.

See [installer guide](docs/INSTALL-PUBLIC.md) for exact steps, install
locations, limitations and Mac security instructions.

### Features

Original Clean/Feedback/Smooth causal compressor styles; Peak/RMS detection;
external mono/stereo sidechain, adjustable detector HP/LP, wet/dry Mix,
Auto Release, adjustable Range, stereo Link and sidechain listen.
46 original factory presets, user preset Save/Open, A/B, APVTS automation
and project restore. The compact 612×580 full GUI has a tall black-glass
transfer/history/meter LCD, brushed graphite chassis and approved silver
hardware-style sliders. MINI and separate Dynamics/Sidechain panels are
available. The host reports **zero additional algorithmic samples**;
actual audio device/driver round-trip latency is separate.

### Platform verification and releases

The cross-platform release workflow builds VST3 and Standalone on Windows,
Linux and macOS, performs automated engine/editor smoke tests, creates
Inno Setup / DEB / portable archive / ad-hoc PKG, and publishes SHA-256
manifests. The initial public tag is a **release candidate** until physical
DAW and audio-interface results, ASIO/JUCE licensing review and (for
frictionless Mac distribution) Developer ID notarization are complete.
A public CI artifact alone is not certified production-stable.

See [Windows physical test quickstart](docs/WINDOWS-QUICKSTART.md),
[hardware readiness](docs/M7-PHYSICAL-READINESS.md), and
[validation](docs/VALIDATION.md). Real-device Maono PS22 crackles/underruns,
32/64/128 buffers, multiple hosts, pluginval, long-session memory and
loudness-matched quality are separate release gates.

### Build locally

CMake 3.22+, C++17, JUCE 8.0.12 pinned to a verified commit.
Visual Studio 2022 C++ Desktop on Windows; Xcode on macOS; JUCE Linux
dependencies from [JUCE official documentation](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Windows Visual Studio uses `cmake -S . -B build -A x64` and
`cmake --build build --config Release`. The Windows Standalone enables
native ASIO by default; review the Steinberg SDK terms when redistributing.

### License and compliance

ArSonComp is AGPL-3.0 licensed; JUCE has its own AGPL/commercial licensing
terms. The Windows Standalone includes Steinberg ASIO SDK-related code
when ARSON_WINDOWS_ASIO=ON. Review its SDK redistribution terms for any
public binary release. No FabFilter code/graphics/algorithms are copied,
and equal sound quality with commercial software is not claimed.
