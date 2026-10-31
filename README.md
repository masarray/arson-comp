# ArSonComp

A compact live compressor: framework-independent C++17 DSP, JUCE VST3 and standalone, zero added algorithmic latency. The interface prioritizes transfer curve, input/output history and gain reduction, with familiar compressor controls and an original ArSon visual identity.

**Status: engineering test build; the integrated full-graph reference layout is under Windows CI verification. No claim of exact sonic/feature parity with FabFilter.**

## Implemented

- Three causal models: Clean feed-forward, Feedback topology, and Smooth dual-envelope response. Peak and 10 ms RMS detection, threshold, ratio, continuously joined soft knee, range, attack, hold and release.
- Optional envelope-dependent auto release; stereo link from independent channels to maximum-linked detection.
- Internal/external mono/stereo sidechain, high-pass/low-pass detector filters and listen.
- Input/makeup gain, dry/wet parallel compression, smoothed bypass, six factory starting points, A/B comparison, and portable Save/Open .arsoncomp preset files (message-thread-only I/O).
- Stable automation IDs and versioned state recall. New model `style` appended without renaming previous parameters. Mono and stereo layouts.
- Separate engine without JUCE, locks, heap allocation, I/O, worker dependencies or delay lines in processing.
- Bounded SPSC telemetry at 100 Hz; editor coalesces updates at 30 Hz, caches static grid, and repaints only the display during metering.
- Full-width integrated compressor graph: responsive input/output/GR history, red transfer curve and dissolving gradient, actual independent stereo LED IN/OUT on the sides, and a distinct GR meter. Dominant Threshold control, red hardware knobs, bottom control strip, and 380px MINI layout. The real threshold marker is draggable via host automation.
- Host parameters are diffed once per block; unchanged input/makeup/filter parameters do not trigger redundant coefficient or exponent recalculation. Peak detection skips RMS square root while its RMS state remains warm.

Zero latency means **0 additional samples from this plugin**, not zero audio-interface/host buffering. Fast attacks can distort low-frequency material; no lookahead means transients can pass. It is a compressor, not a brickwall limiter. The three selectable causal models are Clean feed-forward, Feedback, and Smooth dual-envelope; none uses oversampling, saturation or linear-phase filtering.

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

Double-click knobs to reset; click values to type. Use SAVE and OPEN in the header to export/import current .arsoncomp user presets; those actions perform disk I/O only on the GUI thread. The gold marker in the transfer display can also be dragged to automate Threshold with a single host gesture. MINI switches to a compact, hardware-inspired layout (FULL restores the previous size); this is a GUI-only choice that does not alter audio or latency. Presets do not overwrite external sidechain, listen or bypass. A/B swaps all parameters; it is an editor-session comparison, not a second saved host program.

## Verification and scope

See [architecture](docs/ARD.md), [requirements](docs/PRD.md), [validation](docs/VALIDATION.md) and [handoff](docs/HANDOFF.md). CI runs on relevant PR changes or manual dispatch, cancels superseded builds, caches JUCE source and uploads one installation-ready Windows VST3/standalone test artifact (with screenshots, benchmark, SHA256 checksums and START-HERE instructions). Documentation-only changes do not trigger builds.

Repository license remains AGPL-3.0. JUCE has its own AGPL/commercial terms; see its pinned source license. FabFilter names identify workflow references only; no FabFilter code, assets or algorithm claims are included.

## GUI visual reference and acceptance

The newly redesigned UI uses the user-supplied ArSonComp mockup as a layout acceptance target: integrated upper graph, side meters, red glow and premium hardware controls. No bitmap UI assets, lookahead, worker waits or added algorithmic sample delay. The static graph background builds only on resize and the transfer curve is cached until threshold/ratio/knee/range or width changes. Windows test screenshots must show default 1120x760, minimum 960x670 and MINI 380px. A prior Windows CI build of the portable-preset milestone failed to compile due to incorrectly qualified JUCE XML helper methods; the new milestone fixes these to public AudioProcessor static methods.

## Focused GUI and no-overdraw rendering milestone

The 900x765 default layout replaces the earlier 1120x760 wide editor. Minimum 860x700; the optional 380px MINI/FULL mode retains all audio controls. Header, secondary sliders and toggles now use fixed-reserved regions and short labels such as HP/LP/LINK/EXT SC, with detailed dark contextual tooltips. The full graph is closer to square and remains the hero element. JUCE paint maintains a muted graphite/coral palette with a single enlarged Threshold.

The GUI timer coalesces 100 Hz telemetry at 30 Hz and applies time-normalized attack/release easing only to the **display**, not audio. Input/output and actual GR history use a persistent fixed 400-frame ring buffer, with Path objects rebuilt only on new telemetry or layout change; any UI repaint simply strokes cached paths. Static grid remains cached per resize and transfer curve geometry only rebuilds when its parameters/size change. No new DSP latency, audio-thread worker, lock, heap use or lookahead.

## 2026-10-31: professional graph-edge dissolve and LED render budget

The animated input, output and gain-reduction traces now dissolve on **both horizontal edges**, transitioning from transparent to full brightness by 13.5% of graph width, staying clear until 86.5%, then fading smoothly out. The static transfer curve and its subtle glow use a lower, still visible edge alpha to preserve reading/control affordance. All gradients reuse the same RGB across their stops (avoids muddy fades); the GR activity fill also fades horizontally.

Six four-stop `juce::ColourGradient` brushes are computed only during `resized()` and reused during repaint. The 46-segment **unlit** stereo IN / GR / OUT meter rails now live in the cached static background; `drawMeter()` only draws genuinely **lit** segments and live text. The existing fixed 400-frame history Path cache, 30 Hz paint cadence and SPSC audio telemetry remain in place. No lookahead, worker, heap allocation or lock is introduced in audio processing. JUCE graphics/GUI remains message-thread only.

CI smoke checks numerical alpha symmetry, endpoint/centre contrast and actual ARGB rasterized fade, plus the existing preset round-trip, layout collision, multi-editor and zero-added-sample tests. Use the current CI artifact's Standalone or VST3 to evaluate smoothness and both-line-edge treatment on a physical Windows audio device.

## 2026-10-31 — brushed-metal knobs and decluttered professional layout

All seven primary controls now use an original software-rendered brushed-aluminium knob: a metal outer rim, recessed matte-black LED channel, subtle micro-concentric face texture, a fine indicator needle, and a coral-red active arc. This matches the user's reference **form language**, with the green level arc replaced by our restrained ArSonComp coral. The immutable knob face is cached once per geometric size (two cache slots: regular and oversized Threshold); only the value arc and needle are repainted. This avoids dozens of concentric machining strokes per knob per GUI frame and does not require external artwork or bitmap assets.

The full 900×765 view retains the large transfer/dynamics graph and metering. The header exposes only MINI, Model, Preset, A/B, FILE and Bypass; SAVE/OPEN remain available from the FILE dropdown, with their original asynchronous GUI-thread file dialogs and state-compatible .arsoncomp XML format. The lower strip now has two clear tabs: **DYNAMICS** (Range, Hold, Mix, Link, Auto Release) and **SIDECHAIN** (HP, LP, Peak/RMS, External SC, Listen SC). All underlying DSP parameters and host automation IDs remain attached even while their control page is hidden. Switching pages does not allocate on the audio thread or reconfigure signal routing.

Plugin smoke captures default, minimum, MINI and **Sidechain** layouts; asserts correct visible/hidden controls, main/header overlap constraints, and tab switching at the minimum window size. The GUI renderer and cache are message-thread-only. No new DSP latency, audio worker, lock, or audio-thread allocation.
