# ArSonComp Windows x64 — start here

**Test build / PR #1 — not a production release.** This artifact is generated from the exact commit recorded in `BUILD-INFO.txt` and is validated by Linux engine tests + Windows build, state/editor smoke and benchmark CI. It has **zero added DSP sample latency**; host buffer, audio interface and driver latency still exist.

## A. Fastest test: standalone

Run `Standalone/ArSonComp.exe`. Select the input/output device in the standalone Audio Settings. For monitoring your microphone, enable your input channels and use a low, stable buffer size appropriate to your device. This build does **not** claim ASIO support; host audio-device options vary.

## B. VST3 test in ArSonMiX or another DAW

1. Copy the **entire** folder `Plugin/ArSonComp.vst3` to `C:\Program Files\Common Files\VST3\` (administrator permission may be needed).
2. Restart or rescan VST3 in a compatible host.
3. Insert ArSonComp on a track or bus. Start with ratio 2:1–4:1 and lower Threshold until you see approximately 3–6 dB gain reduction.
4. Compare Clean, Feedback and Smooth at matched output loudness. Try MINI/FULL, drag the white threshold marker, and use SAVE/OPEN for your own `.arsoncomp` preset.
5. Verify external sidechain routing only if your host exposes and connects the sidechain bus. Switch Listen SC off before normal use.

## C. Quick test checklist

- Vocal: listen for pumping, sibilance, level consistency and dry/wet behavior.
- Bass: use low frequencies and aggressive attack to check distortion and filter interactions.
- Transients: check drums with attack/hold/release; zero lookahead is not a true-peak limiter.
- UI: switch MINI/FULL; resize to 740x585 and 780x640, then try a larger window. Confirm bottom LCD value readout, labels, metering, and HP/LP controls.
- Project: save, close, reload; confirm host parameters, style, bypass and input/output routing.
- For performance, use multiple instances in your host and compare CPU with plugin editors hidden/visible.

## Included evidence

`Screenshots/` has Windows editor smoke captures. `benchmark.txt` is a machine-dependent 48-kHz/64-frame microbenchmark and **not** a DAW dropout guarantee. `SHA256SUMS.txt` contains hashes of the executable plugin payloads.

FabFilter Pro-C is an independent benchmark for workflow/quality only. No FabFilter code, UI asset or proprietary algorithm is included.

### New full-graph GUI acceptance

Check upper central transfer graph, red fading curve and live gain reduction line, left input stereo LED meters, right GR and output stereo LEDs, and enlarged Threshold knob. Check the default/minimum/MINI screenshots from this commit, not a previous ZIP. No lookahead is present in the zero-added-sample live signal path.

### Compact responsive GUI and bottom-of-LCD value display

The editor now defaults to **780×640**, with **740×585** minimum full size and **330px** high MINI. FULL restores the preceding expanded dimensions. The primary rotary row is shorter, Sidechain positions adapt to the editor width, and the actual meter/graph canvas retains useful space.

Hover or drag **any** rotary knob or horizontal slider to show its name, exact value and unit in the **bottom centre of the black LCD**. The last focused value remains visible rather than flashing away. MINI uses the right-hand footer for the readout. There are **no overlay value bubbles or TooltipWindow**, so nothing obscures the controls. Double-click a knob or slider to restore its factory value. The JUCE APVTS host-automation attachments and audio DSP remain unchanged.

### Two-sided dissolve, low-cost LED metering

Use the Standalone EXE or VST3 from the **latest** matching Windows Actions artifact. With audio playing, confirm the red gain-reduction trace and gray input/output traces disappear gradually at **both ends** of the dynamics graph without abrupt vertical cutoffs, while remaining crisp in the middle. The curve marker and compressor settings must still work as before. Confirm meters decay smoothly and their unlit segments are visible at all levels. Graph rendering is GUI-only, with no lookahead or additional algorithmic audio samples. Measure CPU and crackles in your own host/driver at 32/64/128 sample buffers rather than treating CI synthetic numbers as real-device measurements.

### Metal controls and decluttered header

All primary knobs now mimic machined/brushed aluminium with a black recessed channel and a coral (not green) value sector. Hover any knob or slider to see its current value in the LCD footer; double-click to restore the default. Click **FILE** in the header to open Save/Open options for your .arsoncomp user presets; the previous separate SAVE/OPEN buttons are now intentionally removed. Switch the footer between **DYNAMICS** and **SIDECHAIN** for advanced detector filters and monitoring. Switching pages does not change or reset audio parameters. Compare all four screenshots (default, minimum, MINI, SIDECHAIN), then validate physically in Standalone or your VST3 host.

### Live-host readiness checks (new)

Run the supplied Standalone on your audio interface and load the complete VST3 in a DAW or ArSonMiX. Start with 48 kHz and a 128-sample host buffer; repeat at 64 then 32 samples where stable. Route a mono kick/mic into the VST3 external Sidechain bus on a stereo track: when **EXT SC** is selected, a strong detector signal must attenuate the main stereo channels while preserving their balance; switching it off must restore internal detection. Try host Bypass at +12 dB Input and Makeup, preset save/reopen, MINI/full, then stop transport and confirm the last dynamics trace clears rather than freezing. Real driver round-trip latency is separate from the plugin's zero additional algorithmic samples.

### Hardware knob and gunmetal slider controls

All rotary knobs retain their coral sector behind the black brushed-metal face and a small coral pointer dot near the rim. The numerical text boxes are removed. Horizontal Range/Hold/Mix/Link/HP/LP faders render a **stationary gunmetal rail**, coral-red recessed level and a **moving gunmetal thumb in front**. They use the same JUCE parameter controls as before, without changing audio behavior. AUTO/EXT SC/LISTEN and ACTIVE/BYPASS use compact left-caption toggles.

### Premium brushed-metal and glass UI

The updated full editor should now have a horizontally brushed grey metal background and a recessed jet-black LCD for the dynamics history/transfer graph and segmented IN/GR/OUT meters. The knob area is shorter, leaving more room for the screen. Range/Hold/Mix/Link/HP/LP rails have a deep shadowed well sized to the original metallic thumb; all toggle captions appear on the left of their switches, including ACTIVE/BYPASS in the header. Both DYNAMICS and SIDECHAIN pages, MINI, old presets/automation and all audio routes continue working. No plugin audio processing or added algorithmic sample delay has changed.
