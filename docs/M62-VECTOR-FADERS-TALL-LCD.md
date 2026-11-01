# M6.2 — Tall LCD and Vector Faders

## Visual changes

- Default full-size window **612 × 580 px** (formerly 612 × 500).
- Minimum **580 × 540 px** (formerly 580 × 480). No increase in width.
- LCD is **326 px high** at default size, versus 246 px before. The actual
  transfer/history plot grows from 186 to 266 px high, a **43% gain**.
- Preserve seven unclipped metal knobs, caption spacing, Dynamics/Sidechain
  controls, and MINI mode (612 × 320 px at default width).
- All **six horizontal sliders** (Range, Hold, Mix, Link, HP, LP) now use
  JUCE Graphics directly: dark graphite inset rail, subtle metallic border,
  coral LED fill, sculpted graphite thumb, short coral grip marker.
- No PNG-based slider track or thumb, no resampling and no allocation/decoding
  in the live audio processing callback.

## Verification and release gate

- Windows: build VST3 + Standalone, smoke CTest and actual snapshot PNGs for
  default full, minimum full, MINI and Sidechain.
- Smoke tests: LCD aspect ratio, exact component geometry, thumb legibility,
  animated rail fill and old M6.1 non-clipped rotary arc bounds.
- Linux: full sanitizer engine-safety suite.
- DSP, preset bank, audio latency, ASIO, automation and routing are unchanged.
  Physical Maono PS22 crackle remains a separate hardware QA requirement.
