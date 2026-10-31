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
