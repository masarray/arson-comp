# Validation record

Initial implementation, 2026-10-31.

Local GCC 13.3 Release engine tests pass: static ratio and knee curves, monotonicity/continuity/range, attack time constants at 44.1/48/96/192 kHz, release time constant, hold duration and adaptive recovery, steady reduction, zero-delay unity impulse, bypass and dry transparency, arbitrary-block partition equivalence, external mono sidechain to stereo, linked/unlinked channels, disconnected sidechain, non-finite containment, zero allocations over 10,000 parameter-update/process blocks, bounded queue saturation and 100,000-frame threaded SPSC order.

ASan/UBSan engine run passes with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer cannot enumerate `/proc/.../task` in this local execution environment; full leak checking passed in the Linux CI job. It is not reported as locally passed.

Local Release throughput example: ~94.9 ns/stereo frame (~0.456% of one 48 kHz real-time core budget), RMS + auto release + high-pass sidechain, 128-sample blocks, sustained sinusoidal input. This is a shared-environment microbenchmark, not a worst-case deadline guarantee, a Windows result or a comparison against Pro-C. Re-run `engine_bench` on the target machine.

Windows MSVC Release VST3 and standalone compilation and engine tests passed in CI run 41132714813. The GUI-smoke script used an incorrect executable name; the correction registers the executable as a CTest target, avoiding hard-coded paths.

Pending until evidence is recorded: editor lifecycle/state test and visual inspection, Windows host/pluginval/hardware tests, listening comparison, leak-check CI. No tested production-ready or Pro-C sonic-parity claim.

## Control-path and GUI milestone (2026-10-31, pending CI)

Source changes: avoid control-rate exponent evaluations for unchanged APVTS values; skip RMS square root on Peak-only samples without freezing the RMS state; preserve automation smoothing and arbitrary-block equivalence. Additional tests cover repeated no-op snapshots, rapid style/detector/filter/bypass/mix automation with no allocation, valid output, three-style 64-sample host-snapshot benchmarks and exact-sized 1040x680 / 920x620 editor snapshots plus 380px compact capture. The threshold line uses one begin/end host gesture per drag, including early editor teardown. No new DSP latency, worker or audio-thread lock.

Current results must be recorded from the CI run for the commit which introduces this section, not inherited from prior runs. GUI captures are functional smoke evidence only until visually reviewed. Pluginval, long-running host stress, listening and Pro-C 3 comparison still pending. References: https://www.fabfilter.com/help/pro-c/using/overview ; https://www.fabfilter.com/help/pro-c/using/displays ; https://www.fabfilter.com/help/pro-c/using/oversampling .
