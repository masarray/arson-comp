# ArSonComp GUI Performance — isolated LCD (2026-10-31)

## User observation
Task Manager screenshot: 0.4% CPU, 46.4 MB process memory and 2.7% GPU at one instant. These are not host callback duration, continuous memory growth, nor proof of a memory leak. Audio stutter cannot be diagnosed from one Task Manager sample.

## Change
- Split live graph/IN/GR/OUT into an **opaque child canvas**. Parent GUI paints the cached brushed chassis, toolbar and fixed hardware controls. Only the graph canvas receives continuous timer invalidation, preventing re-execution of the parent's text, material and header drawing every meter tick.
- Full canvas draws pre-rendered black LCD/grid plus actual signals/meters. MINI repaints only the 47px meter well. All image decoding and path generation remain on JUCE message thread.
- Preserve native threshold drag by forwarding graph canvas mouse coordinates to parent editor. APVTS automation and host callbacks remain untouched.
- Coalesce idle repaint requests when no new SPSC telemetry, no stopped-transport history ageing, and no perceptible meter decay. Keep the 30 Hz live visualization while active.
- Restore the intended 660×490 minimum / 704×520 default resize limits. No new audio worker, mutex, audio allocation, latency, DSP modification, or file I/O from processBlock.

## Test gate
- Actual Windows VST3 + Standalone, JUCE GUI render screenshots at minimum/default/SIDECHAIN/MINI; engine-safety test on Linux.
- Editor smoke asserts the opaque child bounds at 660×490, 704×520 and MINI, plus restored FULL layout.
- Physical Windows interface/DAW comparison is still required for GUI on/off, same driver/buffer and 30–60-minute run; measure actual xrun counters and sustained CPU/GPU and memory, not Task Manager snapshot alone.

Technical reference: JUCE developer ADC 2025 session on component hierarchy, opaque components, cached painting and avoiding repeated work in paint: https://conference.audio.dev/session/2025/understanding-juce-drawing/

## M4 compact and legible isolated LCD

Keep the optimized opaque LCD child and idle-repaint coalescing with the new default 704×520 and minimum 660×490. MINI remains 330px high with a 47px meter well. Captions have higher contrast, separator rules are removed, and the new geometry is covered by actual JUCE visual smoke tests. CI CPU timing is not real-device XRUN proof.

## Compact second pass

Default window reduced from **704×520 to 612×500** (~16% less screen area); minimum 580×480. The seven main knobs are drawn in 62px-high controls while preserving per-control labels, mouse hit targets and the light gunmetal slider rails. All 5 SIDECHAIN elements reflow inside minimum width with 8px gaps. The isolated opaque LCD and idle repaint coalescing remain unchanged; audio DSP does not depend on window size.
