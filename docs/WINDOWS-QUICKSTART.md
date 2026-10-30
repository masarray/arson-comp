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
