# Product requirements

ArSonComp is a lightweight live vocal, karaoke, podcast and mix-bus compressor, delivered as VST3 and standalone. The engine should also be reusable in ArSonMiX without GUI dependencies. Pro-C is the workflow/quality reference, not a basis for asserting sonic identity.

## Initial acceptance

- Mono/stereo host processing, optional mono/stereo external sidechain, zero added sample delay.
- Useful dynamics controls, bounded range, soft knee, Peak/RMS, stereo link and filtered detector audition. Three genuinely different causal models with retained stable IDs and zero sample lookahead.
- Dominant transfer/history display; separate IN/GR/OUT meters; compact, legible knobs, premium dark palette and usable model selector. Verify at minimum/default window sizes.
- Presets with actual parameter values, A/B, host automation and state restoration.
- No heap allocation/lock/wait in DSP; bounded metering and independent GUI lifetime.
- DSP correctness tests, allocation guard, sanitizers, measured throughput, plugin state/editor smoke and Windows artifacts.

## Release gate after initial build

Listen with vocals, music, transients and low-frequency tones. Compare loudness-matched Pro-C references at identical settings, documenting differences. Verify in at least two Windows VST3 hosts, including external sidechain, bypass automation, save/reload, 32/64/128 sample buffers and editor close/reopen. Run pluginval, long-duration stress, and physical-device latency/dropout measurements. No statement of full Pro-C parity or production readiness before these gates.

## Benchmark reference alignment — Pro-C 3, October 2026

The current publicly documented FabFilter Pro-C 3 has 14 styles, optional analog Character, multiple-band detector EQ, Auto Threshold/Gain, surround and optional lookahead/oversampling (see https://www.fabfilter.com/help/pro-c). ArSonComp **does not claim parity** with these features: its three causal models, one-pole HP/LP detector filters and zero-added-sample live path are currently deliberate smaller-scope choices. Prioritize host routing/state/bypass reliability and matched listening before adding analog color or additional topologies. Avoid retrofitting lookahead/oversampling into the zero-latency default; an optional latency-incurring mode would require explicit host compensation and separate acceptance.

## Milestone reconciliation — 2026-10-31

The initial implementation acceptance is **met in automated testing** at code commit `7b340eb2`: C++17 causal DSP with zero added algorithmic samples, Windows VST3/Standalone packaging, APVTS state/preset/automation paths, external sidechain and bypass regressions, both Linux/Windows CI jobs green, and actual Windows GUI screenshots. The compact editor now defaults to **780×640** (minimum **740×585**, MINI height **330**). Hardware-style metal toggle/slider assets and black-glass LCD remain; control values are read from a **single footer label**, not floating popup/tooltip overlays.

**Acceptance deliberately incomplete for production:** pluginval, two independent host applications, a real audio interface, long-session CPU/dropout measurement, 32/64/128 sample buffer tests, host project restore and sidechain automation in the field, DPI scaling, and reference listening. Merging code into `main` establishes a testable engineering baseline only, not a public production release or FabFilter parity.
