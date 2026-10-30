# ArSonComp

A compact live compressor: framework-independent C++17 DSP, JUCE VST3 and standalone, zero added algorithmic latency. The interface prioritizes transfer curve, input/output history and gain reduction, with familiar compressor controls and an original ArSon visual identity.

**Status: engineering test build with three original compression models and a redesigned graphical interface, not claimed to match FabFilter's proprietary algorithms or sound.**

## Implemented

- Three causal models: Clean feed-forward, Feedback topology, and Smooth dual-envelope response. Peak and 10 ms RMS detection, threshold, ratio, continuously joined soft knee, range, attack, hold and release.
- Optional envelope-dependent auto release; stereo link from independent channels to maximum-linked detection.
- Internal/external mono/stereo sidechain, high-pass/low-pass detector filters and listen.
- Input/makeup gain, dry/wet parallel compression, smoothed bypass, six factory starting points and A/B comparison.
- Stable automation IDs and versioned state recall. New model `style` appended without renaming previous parameters. Mono and stereo layouts.
- Separate engine without JUCE, locks, heap allocation, I/O, worker dependencies or delay lines in processing.
- Bounded SPSC telemetry at 100 Hz; editor coalesces updates at 30 Hz, caches static grid, and repaints only the display during metering.
- UI supports a warm graphite/brass/teal vintage theme, direct threshold-marker dragging, and a 380 px compact view without extra DSP work.
- Host parameters are diffed once per block; unchanged input/makeup/filter parameters do not trigger redundant coefficient or exponent recalculation. Peak detection skips RMS square root while its RMS state remains warm.

Zero latency means **0 additional samples from this plugin**, not zero audio-interface/host buffering. Fast attacks can distort low-frequency material; no lookahead means transients can pass. It is a compressor, not a brickwall limiter. The current model is feed-forward, without oversampling, saturation or linear-phase filtering.

## Build

CMake 3.22+, C++17, JUCE pinned to commit `29396c22c93392d6738e021b83196283d6e4d850` (8.0.12).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

Windows: Visual Studio 2022 or newer with C++ desktop tools; add `-A x64` when configuring. Outputs are under `build/ArSonComp_artefacts/Release/{VST3,Standalone}`. Install the **entire** `ArSonComp.vst3` bundle into your VST3 directory, then rescan your host. For standalone, choose the audio device in its audio settings. The initial Windows build uses JUCE's standard device backends; ASIO is not enabled or claimed.

Linux prerequisites: `pkg-config libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev`. A display (or Xvfb) is required for the editor smoke test.

Use `-DJUCE_SOURCE_DIR=/path/to/JUCE` to reuse a local checkout. For fast engine-only development:

```sh
cmake -S . -B build-core -DARSON_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel 2
ctest --test-dir build-core --output-on-failure
./build-core/engine_bench
```

Use `-DARSON_SANITIZE=ON -DCMAKE_BUILD_TYPE=Debug` on GCC/Clang for ASan/UBSan. Do not enable fast-math: non-finite input handling depends on IEEE semantics.

## Live workflow

1. Insert on a mono/stereo channel; start with Default or a vocal preset.
2. Set threshold for the desired gain reduction. Set ratio, attack and release by ear.
3. Use SC high-pass to reduce bass-triggered pumping. End stops bypass each SC filter.
4. Enable External SC only after routing a detector signal in your host. An unconnected external bus receives silence.
5. Use Listen SC to audition the detector, then turn it off. Match output loudness with Makeup before comparing A/B.

Double-click knobs to reset; click values to type. The gold marker in the transfer display can also be dragged to automate Threshold with a single host gesture. MINI switches to a compact, hardware-inspired layout (FULL restores the previous size); this is a GUI-only choice that does not alter audio or latency. Presets do not overwrite external sidechain, listen or bypass. A/B swaps all parameters; it is an editor-session comparison, not a second saved host program.

## Verification and scope

See [architecture](docs/ARD.md), [requirements](docs/PRD.md), [validation](docs/VALIDATION.md) and [handoff](docs/HANDOFF.md). CI runs on relevant PR changes or manual dispatch, cancels superseded builds, caches JUCE source and uploads one Windows VST3/standalone test artifact. Documentation-only changes do not trigger builds.

Repository license remains AGPL-3.0. JUCE has its own AGPL/commercial terms; see its pinned source license. FabFilter names identify workflow references only; no FabFilter code, assets or algorithm claims are included.
