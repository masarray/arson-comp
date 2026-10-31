# ArSonComp engineering contract

- Build a live compressor, not an audio editor. Keep the GUI compact, legible and visually calm. Avoid bulky cards, giant fonts and decorative controls.
- Engine must remain framework independent. Audio thread owns DSP state; no allocation, mutex, file/network I/O or dependency on a worker finishing in process/setParameters.
- One canonical parameter table in engine/Parameters.h. Stable IDs must not be renamed after release. Sanitize outside the inner sample loop.
- Do not add a worker for synchronous audio. Add workers only for demonstrably expensive non-audio tasks, with bounded queues, cancellation and joined lifetime.
- Preserve zero additional latency in the live path. Do not silently introduce lookahead or oversampling.
- Telemetry must be bounded and may drop visual data; never stall audio. GUI changes must not alter DSP results.
- Use RAII and bounded containers. Avoid fast-math, speculative approximation or SIMD unless profiling and numerical tests support it.
- Run meaningful DSP/host tests, inspect GUI renders at minimum/default size, and document unverified hardware behavior. Never claim proprietary Pro-C parity without matched-input audio comparisons.
- Group meaningful changes before CI. Do not dispatch repetitive builds or commit generated binaries/build directories.

## M3 integration release gate
- Always reconcile the GUI development branch and M2 realtime-performance branch before producing a Windows test binary. An artifact from one branch alone is not a combined release.
- Host bypass must pass the first block bit-exactly and must not latch the parameter bypass state. User bypass remains a smoothed, separately attached parameter.
- Build M3 in one PR with both Windows JUCE editor smoke and Linux sanitizer, inspect four actual Windows screenshots and benchmark/stress files before declaring ready-to-test. Do not call CI throughput a hardware xrun proof.

## M4 compact GUI gate
- Default FULL 704x520, minimum 660x490; MINI remains 330 high. Do not increase default GUI size without measuring element collision and user workflow.
- Header has no decorative separator rules. Only MODEL and PRESET need small captions; buttons self-label. Maintain >=4.5:1 contrast between static captions and their dark metal background, using the shared palette contract.
- Inspect screenshots from compiled JUCE at default/minimum, MINI and SIDECHAIN; preserve readable controls, meters, telemetry, preset and APVTS bindings. GUI changes must never modify engine DSP or latency.
