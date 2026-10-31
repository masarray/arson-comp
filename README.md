# ArSonComp

A lightweight **real-time compressor** with a framework-independent C++17 DSP core, JUCE VST3 and Windows Standalone application. Designed for live vocal, karaoke, podcast, music and mix-bus use. This is an **engineering test baseline**, not a certified production release or a claimed FabFilter Pro-C clone.

## Current verified milestone — 31 October 2026

| Area | Implemented and checked |
| --- | --- |
| DSP | Clean feed-forward, causal Feedback and Smooth dual-envelope models; Peak and 10 ms RMS detection; Attack, Release, Hold, Threshold, Ratio, Knee, Range and Auto Release |
| Signal path | Input/Makeup gain, wet/dry Mix, stereo Link, mono/stereo main buses, optional external sidechain, detector HP/LP, SC Listen, smoothed Bypass |
| Latency | **0 additional algorithmic samples** reported by the plugin; host buffer, driver and audio interface latency are separate |
| Host interoperability | Stable APVTS parameter IDs, project state restore, six factory starting presets, user `.arsoncomp` save/open through **FILE**, A/B snapshots, automation attachments |
| GUI | Grey brushed-metal chassis, recessed black LCD graph and IN/GR/OUT meters, photographic black rotary knobs, matched metal toggle PNGs, fixed gunmetal slider rails with moving PNG thumbs and red value fill |
| Value workflow | **No floating value popup/tooltip.** Hover/drag the knobs and sliders to read their value in the LCD's bottom-center footer; MINI uses a small right-side footer |
| Dimensions | Full **780×640** default, **740×585** minimum; MINI **330 px** tall. Resizable full layout and responsive SIDECHAIN row |
| Automated verification | Windows Release VST3 + Standalone, `engine_contract`, `plugin_state_editor`, Linux engine safety/sanitizers, renderer/asset/layout checks, benchmark and install-ready Windows artifact |

**Latest code-tested revision:** `7b340eb2ee5740df13b52b5bfcf984a3b66d0dd3`. Both jobs passed in [GitHub Actions #41202549812](https://github.com/masarray/arson-comp/actions/runs/41202549812). The [Windows x64 artifact](https://github.com/masarray/arson-comp/actions/runs/41202549812/artifacts/13043510391) contains VST3, Standalone, four actual Windows screenshot captures, a benchmark, and SHA-256 manifests. Subsequent documentation-only reconciliation does not modify the compiled source.

### What is *not* verified yet

Real-device ASIO/WASAPI audio latency and stability; two independent DAW/VST3 hosts, external hardware/host sidechain, pluginval, multi-instance long sessions, 125–150% DPI scaling, and loudness-matched listening comparisons. No full Pro-C sonic, feature or production-readiness claim. The initial standalone uses standard JUCE audio device backends; **ASIO support is not claimed**.

## Quick start — Windows

1. Download the Windows artifact linked above and extract its contents.
2. Run `Standalone/ArSonComp.exe`, then choose audio input/output devices in its settings; **or** copy the entire `Plugin/ArSonComp.vst3` folder to `C:\Program Files\Common Files\VST3\`, then rescan your VST3 host.
3. Start with Clean and a moderate preset. Lower Threshold until the GR meter indicates the desired compression. Use Makeup to match audible loudness before A/B or model comparisons.
4. Open **SIDECHAIN** to configure Peak/RMS, HP/LP and optional EXT SC and LISTEN; use **DYNAMICS** for Range, Hold, Mix, Link, Auto Release. FILE provides Save/Open user presets.
5. Hover or move any knob/slider to see name, numerical value and unit in the graph LCD footer. Double-click to restore the parameter's default. Use MINI/FULL to switch form factor.

A disconnected external sidechain may produce silence at the detector. Use LISTEN only to audit the detector and turn it off for normal use. This is a compressor—not a limiter; with no lookahead, transients may pass.

## Build from source

CMake 3.22+, C++17, JUCE 8.0.12 pinned to commit `29396c22c93392d6738e021b83196283d6e4d850`. On Windows, install Visual Studio 2022 or newer with C++ desktop tools.

```sh
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

For engine-only verification on Linux/macOS, omit the Windows `-A x64` argument and use:

```sh
cmake -S . -B build-core -DARSON_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel 2
ctest --test-dir build-core --output-on-failure
./build-core/engine_bench
```

Linux plugin build dependencies depend on the desktop stack, commonly `pkg-config libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev`. Use `-DJUCE_SOURCE_DIR=/path/to/JUCE` to reuse a local checkout. ASan/UBSan engine testing: `-DARSON_SANITIZE=ON -DCMAKE_BUILD_TYPE=Debug` under GCC/Clang. **Do not enable fast-math**, because non-finite handling relies on IEEE floating-point semantics.

## Engineering boundaries

The independent DSP core is free of JUCE. The audio callback does not require a worker, mutex, heap allocation or file/network I/O; it has bounded telemetry. GUI paint is message-thread-only, with cached materials, gradient/grid geometry and 30 Hz display updates. The plugin adds no intentional lookahead, oversampling, linear-phase stage or audio delay line. A reported zero algorithmic samples does **not** mean the full monitoring system has zero latency.

Presets and parameter updates are smoothed where appropriate, but are not promised sample-accurate atomic transactions. Host automation behavior, sidechain audibility, distortion and CPU load must be verified with actual devices.

## Documentation and release gates

- [PRD: scope and acceptance](docs/PRD.md)
- [ARD: DSP, scheduling and GUI ownership](docs/ARD.md)
- [VALIDATION: evidence and outstanding tests](docs/VALIDATION.md)
- [HANDOFF: current next steps and historical milestones](docs/HANDOFF.md)
- [WINDOWS-QUICKSTART: physical test checklist](docs/WINDOWS-QUICKSTART.md)
- [AGENTS: engineering guardrails](AGENTS.md)

Repository license remains AGPL-3.0; JUCE has its own AGPL/commercial terms. FabFilter Pro-C is a third-party workflow and sound-quality reference; no FabFilter asset, code or proprietary algorithm is included.
