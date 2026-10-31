# ArSonComp Windows x64 — start here

**Test build / PR #1 — not a production release.** This artifact is generated from the exact commit recorded in `BUILD-INFO.txt` and is validated by Linux engine tests + Windows build, state/editor smoke and benchmark CI. It has **zero added DSP sample latency**; host buffer, audio interface and driver latency still exist.

## A. Fastest test: standalone

Run `Standalone/ArSonComp.exe`. Select the input/output device in the standalone Audio Settings. For monitoring your microphone, enable your input channels and use a low, stable buffer size appropriate to your device. This build does **not** claim ASIO support; host audio-device options vary.

## B. VST3 test in ArSonMiX or another DAW

1. Copy the **entire** folder `Plugin/ArSonComp.vst3` to `C:\Program Files\Common Files\VST3\` (administrator permission may be needed).
2. Restart or rescan VST3 in a compatible host.
3. Insert ArSonComp on a track or bus. Start with ratio 2:1–4:1 and lower Threshold until you see approximately 3–6 dB gain reduction.
4. Compare Clean, Feedback and Smooth at matched output loudness. Try MINI/FULL, drag the gold threshold marker, and use SAVE/OPEN for your own `.arsoncomp` preset.
5. Verify external sidechain routing only if your host exposes and connects the sidechain bus. Switch Listen SC off before normal use.

## C. Quick test checklist

- Vocal: listen for pumping, sibilance, level consistency and dry/wet behavior.
- Bass: use low frequencies and aggressive attack to check distortion and filter interactions.
- Transients: check drums with attack/hold/release; zero lookahead is not a true-peak limiter.
- UI: switch MINI/FULL; resize to 920x620 and 1040x680; verify labels and metering.
- Project: save, close, reload; confirm host parameters, style, bypass and input/output routing.
- For performance, use multiple instances in your host and compare CPU with plugin editors hidden/visible.

## Included evidence

`Screenshots/` has Windows editor smoke captures. `benchmark.txt` is a machine-dependent 48-kHz/64-frame microbenchmark and **not** a DAW dropout guarantee. `SHA256SUMS.txt` contains hashes of the executable plugin payloads.

FabFilter Pro-C is an independent benchmark for workflow/quality only. No FabFilter code, UI asset or proprietary algorithm is included.

### New full-graph GUI acceptance

Check upper central transfer graph, red fading curve and live gain reduction line, left input stereo LED meters, right GR and output stereo LEDs, and enlarged Threshold knob. Check the default/minimum/MINI screenshots from this commit, not a previous ZIP. No lookahead is present in the zero-added-sample live signal path.

### Focused GUI revision

The current test build opens at 900×765 instead of 1120×760. Minimum resizable size is 860×700; MINI is 380px high, and FULL restores the previous expanded dimensions. Check that all header elements fit, HP/LP and detector AUTO/EXT SC/LISTEN do not collide, and that long explanations appear in tooltips. With a running signal, verify smooth meter/GR motion against the exact audio; smoothing is UI only, with zero extra DSP samples.

### Two-sided dissolve, low-cost LED metering

Use the Standalone EXE or VST3 from the **latest** matching Windows Actions artifact. With audio playing, confirm the red gain-reduction trace and gray input/output traces disappear gradually at **both ends** of the dynamics graph without abrupt vertical cutoffs, while remaining crisp in the middle. The curve marker and compressor settings must still work as before. Confirm meters decay smoothly and their unlit segments are visible at all levels. Graph rendering is GUI-only, with no lookahead or additional algorithmic audio samples. Measure CPU and crackles in your own host/driver at 32/64/128 sample buffers rather than treating CI synthetic numbers as real-device measurements.

### Metal controls and decluttered header

All primary knobs now mimic machined/brushed aluminium with a black recessed channel and a coral (not green) value sector. Hover or double-click any knob to discover its range/reset, or type a numerical value. Click **FILE** in the header to open Save/Open options for your .arsoncomp user presets; the previous separate SAVE/OPEN buttons are now intentionally removed. Switch the footer between **DYNAMICS** and **SIDECHAIN** for advanced detector filters and monitoring. Switching pages does not change or reset audio parameters. Compare all four screenshots (default, minimum, MINI, SIDECHAIN), then validate physically in Standalone or your VST3 host.

### Live-host readiness checks (new)

Run the supplied Standalone on your audio interface and load the complete VST3 in a DAW or ArSonMiX. Start with 48 kHz and a 128-sample host buffer; repeat at 64 then 32 samples where stable. Route a mono kick/mic into the VST3 external Sidechain bus on a stereo track: when **EXT SC** is selected, a strong detector signal must attenuate the main stereo channels while preserving their balance; switching it off must restore internal detection. Try host Bypass at +12 dB Input and Makeup, preset save/reopen, MINI/full, then stop transport and confirm the last dynamics trace clears rather than freezing. Real driver round-trip latency is separate from the plugin's zero additional algorithmic samples.
