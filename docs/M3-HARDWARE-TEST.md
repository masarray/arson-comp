# M3 — Integrated Windows VST3/Standalone hardware test gate

This milestone integrates the finished brushed-metal compact header/toggles with the M2 sparse-ramp real-time compressor optimizations, and makes **host bypass bit-transparent immediately**. The normal *user* bypass toggle remains a smoothed APVTS control; zero algorithmic samples are added.

## Scope and quality status

The plugin ships three independent control models (Clean, Feedback, Smooth), dual detector modes, internal/external sidechain, monitoring, wet/dry, stereo link, and program-dependent release. These are proprietary ArSon algorithms, not FabFilter emulations. [FabFilter Pro-C 3](https://www.fabfilter.com/help/pro-c/using/overview) provides an industry-level comparison target; sonic parity is **not** verified. For live use, keep any future lookahead/oversampling separate and opt-in because they can add latency.

## Install & validate — Windows 64-bit

1. Download the latest **ArSonComp-Windows-x64-ReadyToTest** ZIP from the matching M3 pull-request GitHub Actions run. Extract it. Check `BUILD-INFO.txt` commit SHA and `SHA256SUMS.txt`; do not use an old ZIP from M2.
2. For Standalone, open `Standalone/ArSonComp.exe`. For a DAW/VST3 host, copy the **entire** `Plugin/ArSonComp.vst3` directory into the host's VST3 scan folder (normally `C:\Program Files\Common Files\VST3\`); do not copy only the .vst3 inner DLL.
3. Inspect `Screenshots/` (full, minimum, MINI, sidechain) and the real GUI: embossed brand readable, premium compact header, preset dropdown capped at 176px, switches correct and never overlapped, no tooltip bubble.
4. Start at 48 kHz / 128 sample buffer, then 64, then 32 if stable. Record host, interface/driver version, CPU load, DPC spikes, **actual** host XRUN/dropout counter, and reported plugin latency. Repeat with 1, 8, 16 active plugin instances.
5. Listen on vocals, drums, bass and program bus at matched output loudness; adjust threshold for similar reduction when comparing with a commercial compressor. Assess attacks, recovery, pumping, breath/sibilance, aliasing and dry/wet coherence. Never equate same settings to same tone.
6. Verify host bypass on the **first** processing block with non-unity input/makeup; it should be exactly dry. Toggle the built-in Active/Bypass control separately to assess its intentional short fade; check restart, mono/stereo and external sidechain routing and Listen SC. Test preset save/load and host project restore.
7. Run a 60-minute session; open/close GUI repeatedly, automate threshold/knee, toggle modes and log private-byte trend plus latency/XRUN evidence. Stable memory use and no XRUN on your machine must be observed, not assumed from CI.

## Build evidence

GitHub PR checks gate Linux address/undefined sanitizers and engine contract tests, Windows VST3/Standalone build plus JUCE plugin/editor smoke and screenshot capture, 48kHz engine benchmark, and 32/64/128 × 1/8/24 engine-only realtime stress statistics (mean, P99, P99.9, max). Those stress numbers include hosted CI scheduler noise, **not** a physical audio dropout test.

No worker, mutex, allocation, filesystem access, GUI rendering or telemetry consumer on the audio callback; synchronous DSP state stays audio-thread-owned.
