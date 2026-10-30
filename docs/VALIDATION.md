# Validation record

Initial implementation, 2026-10-31.

Local GCC 13.3 Release engine tests pass: static ratio and knee curves, monotonicity/continuity/range, attack time constants at 44.1/48/96/192 kHz, steady reduction, zero-delay unity impulse, bypass and dry transparency, arbitrary-block partition equivalence, external mono sidechain to stereo, linked/unlinked channels, disconnected sidechain, non-finite containment, zero allocations over 10,000 parameter-update/process blocks, bounded queue saturation and 100,000-frame threaded SPSC order.

ASan/UBSan engine run passes with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer cannot enumerate `/proc/.../task` in this local execution environment; full leak checking is delegated to the Linux CI job, not reported as locally passed.

Local Release throughput example: ~94.9 ns/stereo frame (~0.456% of one 48 kHz real-time core budget), RMS + auto release + high-pass sidechain, 128-sample blocks, sustained sinusoidal input. This is a shared-environment microbenchmark, not a worst-case deadline guarantee, a Windows result or a comparison against Pro-C. Re-run `engine_bench` on the target machine.

Pending until evidence is recorded: full JUCE build, editor visual inspection, Windows host/pluginval/hardware tests, listening comparison, leak-check CI. No tested production-ready or Pro-C sonic-parity claim.
