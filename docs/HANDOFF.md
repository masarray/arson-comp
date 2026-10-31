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
