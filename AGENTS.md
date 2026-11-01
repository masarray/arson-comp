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

## M5 original factory bank contract
- New ArSonComp preset content, not a reproduction of FabFilter proprietary .ffp files/settings or proprietary DSP.
- Catalog browser supports real categories/search/tags/description/favorites without consuming additional permanent GUI space.
- Loading a factory program never changes External SC, Listen or Bypass; factory values are sanitized against the canonical parameter table.
- UI favorites persistence is strictly on the message thread, never in DSP. Test every program for finite output, safe bounds and host parameter recall, plus browser screenshot.

## M7 physical readiness
- Keep the user-approved separate transparent horizontal silver ruler and round thumb; avoid fake light waves or diagonal specular backgrounds. Cache matte brushed grain only on resize.
- Retain visual/native slider hit separation, caption clearances, host automation and LCD meter isolation.
- A shipped Windows test artifact needs the actual screenshot set, QA inventory helper and an explicit physical-test checklist; no CI result proves PS22 xruns or commercial sonic parity.

## Public installer release gate (v0.2)
- Rebuild VST3/Standalone and run actual plugin smoke on all three supported OS runners. Publish only if Windows Inno, Linux DEB+portable and macOS universal ad-hoc packages all pass.
- Inno defaults to user install and supports optional all-users UAC; install to Steinberg-standard VST3 locations, with uninstall and duplicate-scope detection.
- Linux package must provide a user-mode no-sudo option. macOS ad-hoc is **not** Apple Developer ID signing or notarization. Do not instruct users to disable Gatekeeper globally.
- Do not describe a CI-only artifact as hardware-certified stable. Promote the public RC to stable only after license and DAW/interface tests are documented and ARSON_STABLE_APPROVED is explicitly enabled by repository owner.
- Do not remove bundled ASIO support without stating the user impact; check redistribution terms before public stable.
