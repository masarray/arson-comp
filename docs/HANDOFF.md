# Handoff

Start with README, AGENTS, PRD, ARD and VALIDATION. Source of truth is the canonical parameter table, not UI strings. The current milestone delivers a complete first compressor path and UI with explicit release gates; it is not the final parity milestone.

1. Confirm CI artifacts and inspect minimum/default-size GUI renders. Fix functional defects before adding styles.
2. Run pluginval and two-host Windows tests (including ArSonMiX when available). Capture automation, sidechain and project-reload behavior.
3. Filter cutoff/endpoint activation and internal/external detector source now crossfade for 5 ms with regression probes; confirm perceptual quality in a host. Listen and measure release/hold transitions, bypass clicks and low-frequency distortion. Add regression cases for actual findings.
4. Profile with GUI open/closed and many instances at 32/64/128 samples. Optimize only measured hotspots.
5. Work toward Pro-C workflow/quality with documented independent models; never add fake style labels mapped to indistinguishable algorithms.

Keep builds grouped: one relevant-PR workflow with Linux safety and one Windows artifact. No gratuitous matrix or repeated manual runs.
