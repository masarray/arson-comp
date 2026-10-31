# Handoff

Start with README, AGENTS, PRD, ARD and VALIDATION. Source of truth is the canonical parameter table, not UI strings. The current milestone delivers a complete first compressor path and UI with explicit release gates; it is not the final parity milestone.

1. This milestone adds three distinct causal models (Clean feed-forward, Feedback, Smooth dual-pole) and a redesigned meter/transfer-centric GUI, without adding samples of latency. Next batch optimizes unchanged host snapshots and Peak RMS meter CPU, adds direct transfer-marker threshold editing with one automation gesture and a fully reflowed 380 px vintage compact GUI. Smoke screenshots now cover default, minimum and compact sizes; visually inspect all three in Windows artifacts. Confirm the new CI artifact and visually inspect minimum/default-size GUI PNGs. Review labels, overlap and readability in real Windows scaling.
2. Run pluginval and two-host Windows tests (including ArSonMiX when available). Capture automation, sidechain and project-reload behavior.
3. Filter cutoff/endpoint activation and internal/external detector source now crossfade for 5 ms with regression probes; confirm perceptual quality in a host. Listen and measure release/hold transitions, bypass clicks and low-frequency distortion. Add regression cases for actual findings.
4. Profile with GUI open/closed and many instances at 32/64/128 samples. Optimize only measured hotspots.
5. Work toward Pro-C workflow/quality with documented independent models; never add fake style labels mapped to indistinguishable algorithms.

The next batch addresses fast real-world testing: a single Windows artifact rooted at Plugin/, Standalone/, Screenshots/, plus SHA256 checksums and a quickstart; SAVE/OPEN provides versioned portable preset XML outside the audio callback, with automated roundtrip testing. MINI meter labels are separated from bars for readability. Verify the new UI Windows artifact before promoting the build.

Keep builds grouped: one relevant-PR workflow with Linux safety and one Windows artifact. No gratuitous matrix or repeated manual runs.

## Current priority: reference visual fidelity + previous build regression

The supplied red ArSonComp mockup is now the acceptance layout for a full-width integrated transfer/compression graph, side LED meters with true stereo peaks, animated actual GR, red dissolving fill, dominant Threshold knob, and two-row compact controls. Preserve no-lookahead live mode. IMPORTANT: CI for f847fa6 failed because PresetIO.h incorrectly called juce namespace functions getXmlFromBinary/copyXmlToBinary. Correct public API is juce::AudioProcessor::getXmlFromBinary/copyXmlToBinary. Validate Windows compile and the three screenshot variants before claiming milestone completed. Keep one grouped commit/CI and retain Draft until real verification.

## Focused-layout milestone handoff

The full reference GUI in commit f63e8c6 passed Windows/Linux CI, but user feedback on actual screenshot showed it was too wide and secondary controls were crowded. The new grouped follow-up moves to 900x765 default (860x700 minimum), shortens secondary labels with tooltip help, reflows all header/buttons/slider regions and adds smoke collision bounds tests. It also caches 400-point signal/GR Path objects between actual telemetry updates and applies UI-only time-based meter easing. Do not move easing onto the audio callback or add Lookahead. Review default/minimum/MINI Windows screenshots before release; host/pluginval/real device tests remain outstanding.

## Edge dissolve and physical-test handoff

The latest grouped GUI milestone applies symmetric horizontal transparency fades to input/output/GR history and keeps the transfer curve readable with a lower-opacity edge; cache the six gradient brushes and meter rails on resize. The new `GraphPaint.h` helper is GUI-only; numerical and ARGB rasterizer smoke asserts actual fades. Meter-active segments are rendered at 30Hz over pre-rendered inactive rails; do not recreate LED background rectangles every frame. There must be **no** DSP engine changes or new audio-thread visual work.

Build and visually inspect 900x765 default, 860x700 minimum and MINI 380 screenshots in the Windows Actions artifact before distributing. Verify live rendering on the user's real Windows host/ASIO interface and DAW/ArSonMiX, check visual motion and actual CPU usage, channel independence, host automation, and session reopen. Running an editor smoke in CI does not replace a physical-device validation.

## Hardware knob and progressive disclosure handoff

The user supplied a concentric brushed-metal/black-groove rotary-knob image with a green indicator sector and asked for matching physical feel in the existing coral ArSonComp palette. Implementation: UI-only immutable face cache (regular + larger Threshold), concentric machined microtexture generated on size change, subtle metal rim and black inset ring, coral active groove and understated needle. Do **not** use the supplied source image as a runtime asset or perform high-cost graphics inside DSP callbacks.

To address overcrowding, SAVE/OPEN are moved behind FILE (same async dialogs), header retains Model/Preset/A-B/Bypass/MINI; lower strip uses DYNAMICS vs SIDECHAIN views with all APVTS attachments still alive. Default DYNAMICS displays Range/Hold/Mix/Link/AUTO; SIDECHAIN displays HP/LP, Peak/RMS, External SC and Listen SC. Switching is UI-only. A new CI screenshot `ArSonComp-preview-sidechain.png` is captured at minimum size. Existing default/minimum/MINI screenshots persist. Inspect screenshots, verify text boxes and knobs align with labels, and no tab controls overlap. Validate in VST3/Standalone host, not solely GUI smoke.

One grouped commit should produce one Linux/Windows CI run; keep PR draft until new CI and real device validation.

## Black-metal photographic asset handoff

The latest requested user image is a lighter silver radial-brushed-metal dial. A black/gunmetal reinterpretation was generated, and a compact 96×96 RGB JPEG is now included under `assets/black_brushed_knob.jpg` for efficient JUCE BinaryData inclusion. Compared with the prior vector-only knob, this keeps real radial studio-light reflection, fitted to the user's image. The knob JPEG is decoded once on the GUI thread, circular-clipped and cached at actual UI sizes; no image work leaks into the audio callback. The large original transparent PNG can be distributed as a design master separately. Test Windows screenshots at 900×765 and 860×700, verify the black metal body isn't hidden behind the coral arc, and ensure all dynamics/sidechain and MINI states still function. Do not merge draft before CI and real-device tests.

## Host routing / transport-stop milestone

The previous black-metal image VST3+Standalone build `4e4553e` and its Linux/Windows CI are green, with a verified artifact. Subsequent grouped changes add (1) actual JUCE host-style sidechain/mono/bypass regression at 32/64/128 sample frames; (2) bounded UI-only silent-history ageing so stopping transport clears stale traces rather than freezing them. All DSP math, meter producer, host parameter IDs, and audio callback allocation behavior are left intact. Test Windows screenshots normal/min/MINI/SIDECHAIN plus host bypass/sidechain before claiming hardware readiness. Reference Pro-C 3 for feature/workflow direction, not identical sonic behavior.

## Latest requested premium surface polish

The customer screenshot showed a red ring drawn **on top of** an oversized black-metal knob, a near-center white pointer, cluttered numerical textboxes, checkbox visuals and a misaligned ACTIVE header. One coordinated GUI milestone fixes all together: behind-the-metal enlarged coral arc, small rim-adjacent coral dot, native drag value bubble + dynamically formatted dark tooltip instead of permanent numeric boxes, custom minified toggle tracks with captions below, horizontally aligned header ACTIVE/BYPASS switch and label, and the actual supplied `PAN-SLIDER.png` as the small slider thumb (18×40 source scaled to 11×22).

Do not conflate code completion with physical readiness. The prior `805b7dd` Windows CI failed in `plugin_state_editor` during new host-routing checks (engine-safety passed), so this batch also adds identifiable `HOST_BUS_INTEGRATION_FAILED N` logging and the current branch CI must be green before distributing a new ZIP. Avoid triggering multiple full JUCE builds for aesthetic tweaks; inspect the four actual screenshots and test the toggle interactions, presets, host automation, 32/64/128 frame routing before claiming parity.

## Brushed-metal chassis & glass LCD — UI material milestone

Requested from approved 1536x1024 visual reference: (1) brushed grey metal body; (2) black shiny glass scope; (3) matching black LCD meter panels; (4) recessed shadowed slider gutters as high as the metallic slider thumbs; (5) captions on the **left** of the toggle; (6) tighter knob block.

Implemented as one grouped UI-only milestone, with a dedicated reusable `SurfacePaint.h` and existing `GraphPaint.h` meter rails. Full view uses a **cached** 900x765 brushed chassis and separate cached black display/grid. MINI's black glass meter bezel is cached. Original black knob asset remains; main knob component height drops 105->82. LCD enlarges; footer slides up, slider components shrink to 40px with a 26px recessed well. Native ToggleButton APVTS attachments and caption semantics unchanged.

Check Windows CI smoke (actual JUCE graphics RGB contrast, bound checks, DEFAULT/MINIMUM/MINI/SIDECHAIN screenshots), confirm text legibility against grey steel, inspect graph as true black glass rather than dark grey, and test host sample rates/32–128 buffer/automation. This is visual refinement only, not Pro-C feature or sonic equivalence. Keep PR Draft until physical testing and pluginval.

## Latest gunmetal slider-track + sliding-thumb milestone

User explicitly requested combining the TWO preceding approved graphic assets into one physically convincing JUCE slider: fixed gunmetal horizontal track, independently movable upright thumb over it, and a red LED that tracks the parameter value INSIDE the rail. This is implemented as a **layered control**, not as one static composited picture. The user's earlier glowing thumb image supplies the red vertical slit in the thumb, while the central horizontal groove fills from left to current value dynamically. CMake compiles optimized transparent images into VST3 and Standalone; no external asset path. Integrated smoke verifies both resources, 3 rendered positions, LED off/on and thumb travel. All audio DSP is unchanged.

After CI, inspect actual Windows 900×765, 860×700, MINI, SIDECHAIN screenshots. Toggle back and forth between 0%, 50%, 100% using Range/Mix/Link and HP/LP. Check that the track remains stationary, thumb is foremost, the red LED never spills outside the inset channel, full range and drag area work with mouse and host automation. PR remains Draft until host + physical device tests.
