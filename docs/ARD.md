# Architecture

## Ownership and scheduling

`engine/` is a C++17 static library; `plugin/Processor` adapts host buses, parameters and state; `plugin/Editor` owns presentation. DSP lifetime is independent of editor lifetime. No worker is needed for this bounded, causal processing workload.

The processor caches APVTS atomic pointers once, then snapshots parameters once per block. Parameters are clamped/canonicalized; expensive time/filter coefficients only change when their parameter changes. Input/makeup dB conversion occurs at control updates and linear gain ramps run in audio. Five-millisecond ramps protect continuous gain/curve/mix and boolean transitions. Attack/release coefficient updates preserve the current envelope. Sidechain filter coefficients update at block boundaries; rapid cutoff automation during Listen SC still needs listening validation.

Signal flow: input trim → detector tap → optional detector HP/LP → Peak/RMS blend → channel linking → soft-knee static reduction → attack/hold/release envelope → gain application → parallel mix → SC listen crossfade → bypass crossfade. Dry/wet paths share the same sample index. Bypass returns original input after its short transition. Non-finite input is replaced with silence; input magnitudes beyond ±32 are bounded for numerical containment.

RMS detector uses a 10 ms exponential energy estimate. Attack/release are 63.2% time constants in the dB reduction domain. Auto release blends base and 4× slower release coefficients according to existing reduction. This is an explicit independent design, not a recreation of FabFilter's undisclosed program-dependent models. The first-order detector HP/LP are bypassed at 20 Hz / 20 kHz endpoints. Stereo linking blends each detector toward the maximum channel level.

## Data exchange

A fixed 512-slot SPSC queue transfers 10 ms maxima (input, output, reduction). Acquire/release publication protects non-atomic slots; read/write indices occupy distinct cache lines. Audio drops frames if full. There is exactly one editor consumer. Editor drains at most 512 frames per 30 Hz tick, updates its fixed 400-frame history and coalesces meter repaint. Reopening discards a bounded stale backlog. Static display grid is rebuilt on resize, not on audio or every timer tick.

APVTS parameter atomics are independent, so bulk preset/state changes can be observed across adjacent audio blocks; five-millisecond ramps limit transition artifacts. They are not claimed to be a transactional sample-accurate preset switch. Host automation is block-rate in this implementation. State XML serialization is outside processBlock. Schema 1 rejects unknown root/schema and restores finite, clamped known values.

RAII owns GUI attachments and buffers. Timer is stopped before editor teardown. No process-global mutable DSP state. Audio never accesses editor objects. Memory use does not grow with session duration.

## Reference decisions

- FabFilter Pro-C 2 manual: https://www.fabfilter.com/downloads/pdf/help/ffproc2-manual.pdf — workflow references for dynamics/time controls, sidechain, metering, lookahead and oversampling. Lookahead/oversampling intentionally absent from zero-latency live path.
- JUCE APVTS documentation: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html — raw atomic parameter reads and state/attachment separation.
- JUCE source pinned at 8.0.12: https://github.com/juce-framework/JUCE/tree/29396c22c93392d6738e021b83196283d6e4d850 — build reproducibility, not an assertion this is the newest release.

## Deferred, not represented as working

Pro-C-specific styles, mid/side compression, frequency-dependent linking, auto makeup, lookahead/offline quality mode, oversampling, scalable vector accessibility polish, user preset files, ASIO standalone integration and measured proprietary sound matching. Future modes must declare latency and pass block/automation tests separately.
