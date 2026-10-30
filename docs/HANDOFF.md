# Handoff

Start with README, AGENTS, PRD, ARD and VALIDATION. Source of truth is the canonical parameter table, not UI strings. The current milestone delivers a complete first compressor path and UI with explicit release gates; it is not the final parity milestone.

1. This milestone adds three distinct causal models (Clean feed-forward, Feedback, Smooth dual-pole) and a redesigned meter/transfer-centric GUI, without adding samples of latency. Next batch optimizes unchanged host snapshots and Peak RMS meter CPU, adds direct transfer-marker threshold editing with one automation gesture and a fully reflowed 380 px vintage compact GUI. Smoke screenshots now cover default, minimum and compact sizes; visually inspect all three in Windows artifacts. Confirm the new CI artifact and visually inspect minimum/default-size GUI PNGs. Review labels, overlap and readability in real Windows scaling.
2. Run pluginval and two-host Windows tests (including ArSonMiX when available). Capture automation, sidechain and project-reload behavior.
3. Filter cutoff/endpoint activation and internal/external detector source now crossfade for 5 ms with regression probes; confirm perceptual quality in a host. Listen and measure release/hold transitions, bypass clicks and low-frequency distortion. Add regression cases for actual findings.
4. Profile with GUI open/closed and many instances at 32/64/128 samples. Optimize only measured hotspots.
5. Work toward Pro-C workflow/quality with documented independent models; never add fake style labels mapped to indistinguishable algorithms.

Keep builds grouped: one relevant-PR workflow with Linux safety and one Windows artifact. No gratuitous matrix or repeated manual runs.
