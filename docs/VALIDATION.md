# Validation record

Initial implementation, 2026-10-31.

Local GCC 13.3 Release engine tests pass: static ratio and knee curves, monotonicity/continuity/range, attack time constants at 44.1/48/96/192 kHz, release time constant, hold duration and adaptive recovery, steady reduction, zero-delay unity impulse, bypass and dry transparency, arbitrary-block partition equivalence, external mono sidechain to stereo, linked/unlinked channels, disconnected sidechain, non-finite containment, zero allocations over 10,000 parameter-update/process blocks, bounded queue saturation and 100,000-frame threaded SPSC order.

ASan/UBSan engine run passes with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer cannot enumerate `/proc/.../task` in this local execution environment; full leak checking passed in the Linux CI job. It is not reported as locally passed.

Local Release throughput example: ~94.9 ns/stereo frame (~0.456% of one 48 kHz real-time core budget), RMS + auto release + high-pass sidechain, 128-sample blocks, sustained sinusoidal input. This is a shared-environment microbenchmark, not a worst-case deadline guarantee, a Windows result or a comparison against Pro-C. Re-run `engine_bench` on the target machine.

Windows MSVC Release VST3 and standalone compilation and engine tests passed in CI run 41132714813. The GUI-smoke script used an incorrect executable name; the correction registers the executable as a CTest target, avoiding hard-coded paths.

Pending until evidence is recorded: editor lifecycle/state test and visual inspection, Windows host/pluginval/hardware tests, listening comparison, leak-check CI. No tested production-ready or Pro-C sonic-parity claim.

## Control-path and GUI milestone (2026-10-31, pending CI)

Source changes: avoid control-rate exponent evaluations for unchanged APVTS values; skip RMS square root on Peak-only samples without freezing the RMS state; preserve automation smoothing and arbitrary-block equivalence. Additional tests cover repeated no-op snapshots, rapid style/detector/filter/bypass/mix automation with no allocation, valid output, three-style 64-sample host-snapshot benchmarks and exact-sized 1040x680 / 920x620 editor snapshots plus 380px compact capture. The threshold line uses one begin/end host gesture per drag, including early editor teardown. No new DSP latency, worker or audio-thread lock.

Current results must be recorded from the CI run for the commit which introduces this section, not inherited from prior runs. GUI captures are functional smoke evidence only until visually reviewed. Pluginval, long-running host stress, listening and Pro-C 3 comparison still pending. References: https://www.fabfilter.com/help/pro-c/using/overview ; https://www.fabfilter.com/help/pro-c/using/displays ; https://www.fabfilter.com/help/pro-c/using/oversampling .

## Integrated-graph reference milestone (pending CI)

Verify default 1120x760, minimum 960x670 and compact 380px Windows screenshot geometry; confirm accurate dark/red gradients, animated reduction/input/output history on a single canvas, red curved transfer law, dominant Threshold knob, stereo IN L/R and OUT L/R LED meters and GR meter. Engine checks 0.2/0.65 independent stereo input/output peaks and preserved aggregate maximum. Preset smoke must pass after correcting public static AudioProcessor XML helpers. No false lookahead knob, no added DSP samples. Host validation and DPI scaling remain release gates; screenshot smoke alone does not prove sonic parity.

## Focused GUI milestone — acceptance

- Assert 900×765 default, 860×700 minimum, 380px MINI and FULL roundtrip in Windows VST3 editor smoke.
- Check no collision / no off-window bounds for model+preset+SAVE+OPEN, 7 main controls, 6 secondary controls and detector/AUTO/EXT SC/LISTEN group; verify labels and tooltip visual design.
- Screenshot visual review: graph aspect and central prominence, legible 6 dB grid, restrained coral dissolving fill, stereo LED meters, large Threshold, full/mini readability.
- UI 30 Hz timing and 100 Hz bounded queue; time-based meter easing only on GUI; fixed ring history paths reused across paints and recomputed only for new telemetry or resize. Confirm on host with audio that motion is smoother without extra DSP latency.
- No engine code changed on this milestone; zero added sample contract preserved. Benchmark multiple editors/instances and measure CPU only from real device when available. pluginval / 150% DPI still pending.

## Graph-edge dissolve — required validation

- JUCE `ColourGradient::addColour` four-stop alpha unit test: transparent endpoints, full centre, mirror-like left/right falloffs; transfer retains nonzero edge alpha for readability.
- Raster test: draw line using real JUCE ARGB `Graphics`, check centre alpha > 0.70 and both edge samples < 35% of centre; no bitmap masks or CPU blur.
- Assert full/minimum/MINI editor snapshots, preset save/reload, automation ID stability, no overlap and independent stereo metering via existing tests.
- Static segmented meter rails cached in resized image; repaint draws **only active** segments plus labels/values. Confirm no off-bars ghosting with host running then stopping and resizing.
- No engine changes: existing Linux sanitizer suite proves zero additional DSP samples, bounded SPSC and no audio-thread allocation. Pending real-device CPU/latency/pluginval and 150% DPI.

## Brushed-metal knob and control-page validation

- Actual Windows screenshot, 900×765 and 860×700: check metallic beveled ring and subtle fine machined concentric grain, coral arc in recessed groove, legible indicator and editable text values. Confirm enlarged Threshold does not collide with Ratio/Release. Ensure no fake skeuomorphic gradients/glows dominate readability.
- Verify FILE button opens Save/Open actions, .arsoncomp roundtrip remains valid and A/B/bypass remain visible. Hidden SAVE/OPEN controls must not render, but their callbacks remain functional through FILE.
- Verify default DYNAMICS page shows Range, Hold, Mix, Link and AUTO and hides detector/HP/LP/EXT/LISTEN. Switch to SIDECHAIN and verify complementary visibility plus zero overlap, then restore; repeat at min 860×700 and MINI 380px.
- CI emits a fourth SIDECHAIN screenshot, additional to default/minimum/MINI; use screenshot review to identify any actual clipping. Knob cache is allocated only on GUI thread, at most once per changed diameter per role; all animation/LED metering remains unchanged.
- Pro-C-grade DSP parity, pluginval, multi-instance real CPU, host automation reliability and hardware 32/64/128 sample tests remain release gates.

## Live host-signal and transport-stop acceptance

- Windows `plugin_state_editor` now uses JUCE's *real bus layout*: stereo main + mono external sidechain at 32/64/128 frames. Verify after 1 s of main 0.05/0.025 and sidechain 1.0 that output is close to the predicted 15 dB reduction (threshold -20 dB, 4:1 hard knee), side input remains 1.0 and stereo ratio remains 2:1.
- Toggle external-to-internal with unchanged sidechain bus: signal must recover to 0.05/0.025. Assert host bypass restores unchanged input after smoothing, ignores +12 dB Input/Makeup, and reports zero algorithmic sample latency.
- Validate mono main, no external SC and 1:1 unity ratio, at 32 samples. No host-bus memory boundary crossing.
- Engine regression checks UI-only 16-frame ring ageing: 6 frames first, exactly 10 remaining, no more writes after the ring is cleared. Playback must not display frozen GR history after host stops; physical latency/dropout validation is still pending.

## Premium small-control render acceptance

- Windows screenshots show coral value sector visibly **outside/behind** the black-metal dial at top 900×765 and minimum 860×700; dot appears near the face rim and no central white needle remains. Dynamic graph, stereo meters and threshold interaction unchanged.
- No persistent value boxes on any of the 13 sliders. Hover provides the current numeric value and unit via a dark tooltip; dragging shows JUCE value popup. Double-click reset and APVTS host automation still function.
- All previous checkbox surfaces must render as compact capsule switches. DYNAMICS AUTO and SIDECHAIN EXT/LISTEN captions sit underneath; header ACTIVE text must not overlap the switch and must change to BYPASS when bypassed.
- Supplied source `PAN-SLIDER.png` is in `ArSonCompKnobAssets`; smoke asserts it decodes as 18×40 and contains transparent corners plus opaque center. Thumb is 11×22 in render.
- All prior host bus integration failure branches now emit a numeric `HOST_BUS_INTEGRATION_FAILED` diagnostic. Failures are not suppressed; no claim of new Windows artifact until that test and engine contract both pass.
